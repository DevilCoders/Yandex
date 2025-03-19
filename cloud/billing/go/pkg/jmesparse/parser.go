package jmesparse

import (
	"encoding/json"
	"fmt"
	"strconv"
	"strings"
	"unicode"
)

type astNodeType int

//go:generate stringer -type astNodeType
const (
	ASTEmpty astNodeType = iota
	ASTComparator
	ASTCurrentNode
	ASTExpRef
	ASTFunctionExpression
	ASTField
	ASTFilterProjection
	ASTFlatten
	ASTIdentity
	ASTIndex
	ASTIndexExpression
	ASTKeyValPair
	ASTLiteral
	ASTMultiSelectHash
	ASTMultiSelectList
	ASTOrExpression
	ASTAndExpression
	ASTNotExpression
	ASTPipe
	ASTProjection
	ASTSubexpression
	ASTSlice
	ASTValueProjection
)

// ASTNode represents the abstract syntax tree of a JMESPath expression.
type ASTNode struct {
	NodeType astNodeType
	Value    interface{}
	Children []ASTNode
}

func (node ASTNode) String() string {
	return node.PrettyPrint(0)
}

// PrettyPrint will pretty print the parsed AST.
// The AST is an implementation detail and this pretty print
// function is provided as a convenience method to help with
// debugging.  You should not rely on its output as the internal
// structure of the AST may change at any time.
func (node ASTNode) PrettyPrint(indent int) string {
	spaces := strings.Repeat(" ", indent)
	output := fmt.Sprintf("%s%s {\n", spaces, node.NodeType)
	nextIndent := indent + 2
	if node.Value != nil {
		if converted, ok := node.Value.(fmt.Stringer); ok {
			// Account for things like comparator nodes
			// that are enums with a String() method.
			output += fmt.Sprintf("%svalue: %s\n", strings.Repeat(" ", nextIndent), converted.String())
		} else {
			output += fmt.Sprintf("%svalue: %#v\n", strings.Repeat(" ", nextIndent), node.Value)
		}
	}
	lastIndex := len(node.Children)
	if lastIndex > 0 {
		output += fmt.Sprintf("%schildren: {\n", strings.Repeat(" ", nextIndent))
		childIndent := nextIndent + 2
		for _, elem := range node.Children {
			output += elem.PrettyPrint(childIndent)
		}
	}
	output += fmt.Sprintf("%s}\n", spaces)
	return output
}

var bindingPowers = map[TokType]int{
	TEOF:                0,
	TUnquotedIdentifier: 0,
	TQuotedIdentifier:   0,
	TRbracket:           0,
	TRparen:             0,
	TComma:              0,
	TRbrace:             0,
	TNumber:             0,
	TCurrent:            0,
	TExpref:             0,
	TColon:              0,
	TPipe:               1,
	TOr:                 2,
	TAnd:                3,
	TEQ:                 5,
	TLT:                 5,
	TLTE:                5,
	TGT:                 5,
	TGTE:                5,
	TNE:                 5,
	TFlatten:            9,
	TStar:               20,
	TFilter:             21,
	TDot:                40,
	TNot:                45,
	TLbrace:             50,
	TLbracket:           55,
	TLparen:             60,
}

// Parser holds state about the current expression being parsed.
type Parser struct {
	expression string
	tokens     []Token
	index      int
}

// NewParser creates a new JMESPath parser.
func NewParser() *Parser {
	p := Parser{}
	return &p
}

// Parse will compile a JMESPath expression.
func (p *Parser) Parse(expression string) (ASTNode, error) {
	lexer := NewLexer()
	p.expression = expression
	p.index = 0
	tokens, err := lexer.tokenize(expression)
	if err != nil {
		return ASTNode{}, err
	}
	p.tokens = tokens
	parsed, err := p.parseExpression(0)
	if err != nil {
		return ASTNode{}, err
	}
	if p.current() != TEOF {
		return ASTNode{}, p.syntaxError(fmt.Sprintf(
			"Unexpected token at the end of the expression: %s", p.current()))
	}
	return parsed, nil
}

func (p *Parser) parseExpression(bindingPower int) (ASTNode, error) {
	var err error
	leftToken := p.lookaheadToken(0)
	p.advance()
	leftNode, err := p.nud(leftToken)
	if err != nil {
		return ASTNode{}, err
	}
	currentToken := p.current()
	for bindingPower < bindingPowers[currentToken] {
		p.advance()
		leftNode, err = p.led(currentToken, leftNode)
		if err != nil {
			return ASTNode{}, err
		}
		currentToken = p.current()
	}
	return leftNode, nil
}

func (p *Parser) parseIndexExpression() (ASTNode, error) {
	if p.lookahead(0) == TColon || p.lookahead(1) == TColon {
		return p.parseSliceExpression()
	}
	indexStr := p.lookaheadToken(0).Value
	parsedInt, err := strconv.Atoi(indexStr)
	if err != nil {
		return ASTNode{}, err
	}
	indexNode := ASTNode{NodeType: ASTIndex, Value: parsedInt}
	p.advance()
	if err := p.match(TRbracket); err != nil {
		return ASTNode{}, err
	}
	return indexNode, nil
}

func (p *Parser) parseSliceExpression() (ASTNode, error) {
	parts := []*int{nil, nil, nil}
	index := 0
	current := p.current()
	for current != TRbracket && index < 3 {
		if current == TColon {
			index++
			p.advance()
		} else if current == TNumber {
			parsedInt, err := strconv.Atoi(p.lookaheadToken(0).Value)
			if err != nil {
				return ASTNode{}, err
			}
			parts[index] = &parsedInt
			p.advance()
		} else {
			return ASTNode{}, p.syntaxError(
				"Expected tColon or tNumber" + ", received: " + p.current().String())
		}
		current = p.current()
	}
	if err := p.match(TRbracket); err != nil {
		return ASTNode{}, err
	}
	return ASTNode{
		NodeType: ASTSlice,
		Value:    parts,
	}, nil
}

func (p *Parser) match(tokenType TokType) error {
	if p.current() == tokenType {
		p.advance()
		return nil
	}
	return p.syntaxError("Expected " + tokenType.String() + ", received: " + p.current().String())
}

func (p *Parser) led(tokenType TokType, node ASTNode) (ASTNode, error) {
	switch tokenType {
	case TDot:
		if p.current() != TStar {
			right, err := p.parseDotRHS(bindingPowers[TDot])
			return ASTNode{
				NodeType: ASTSubexpression,
				Children: []ASTNode{node, right},
			}, err
		}
		p.advance()
		right, err := p.parseProjectionRHS(bindingPowers[TDot])
		return ASTNode{
			NodeType: ASTValueProjection,
			Children: []ASTNode{node, right},
		}, err
	case TPipe:
		right, err := p.parseExpression(bindingPowers[TPipe])
		return ASTNode{NodeType: ASTPipe, Children: []ASTNode{node, right}}, err
	case TOr:
		right, err := p.parseExpression(bindingPowers[TOr])
		return ASTNode{NodeType: ASTOrExpression, Children: []ASTNode{node, right}}, err
	case TAnd:
		right, err := p.parseExpression(bindingPowers[TAnd])
		return ASTNode{NodeType: ASTAndExpression, Children: []ASTNode{node, right}}, err
	case TLparen:
		name := node.Value
		var args []ASTNode
		for p.current() != TRparen {
			expression, err := p.parseExpression(0)
			if err != nil {
				return ASTNode{}, err
			}
			if p.current() == TComma {
				if err := p.match(TComma); err != nil {
					return ASTNode{}, err
				}
			}
			args = append(args, expression)
		}
		if err := p.match(TRparen); err != nil {
			return ASTNode{}, err
		}
		return ASTNode{
			NodeType: ASTFunctionExpression,
			Value:    name,
			Children: args,
		}, nil
	case TFilter:
		return p.parseFilter(node)
	case TFlatten:
		left := ASTNode{NodeType: ASTFlatten, Children: []ASTNode{node}}
		right, err := p.parseProjectionRHS(bindingPowers[TFlatten])
		return ASTNode{
			NodeType: ASTProjection,
			Children: []ASTNode{left, right},
		}, err
	case TEQ, TNE, TGT, TGTE, TLT, TLTE:
		right, err := p.parseExpression(bindingPowers[tokenType])
		if err != nil {
			return ASTNode{}, err
		}
		return ASTNode{
			NodeType: ASTComparator,
			Value:    tokenType,
			Children: []ASTNode{node, right},
		}, nil
	case TLbracket:
		tokenType := p.current()
		var right ASTNode
		var err error
		if tokenType == TNumber || tokenType == TColon {
			right, err = p.parseIndexExpression()
			if err != nil {
				return ASTNode{}, err
			}
			return p.projectIfSlice(node, right)
		}
		// Otherwise this is a projection.
		if err := p.match(TStar); err != nil {
			return ASTNode{}, err
		}
		if err := p.match(TRbracket); err != nil {
			return ASTNode{}, err
		}
		right, err = p.parseProjectionRHS(bindingPowers[TStar])
		if err != nil {
			return ASTNode{}, err
		}
		return ASTNode{
			NodeType: ASTProjection,
			Children: []ASTNode{node, right},
		}, nil
	}
	return ASTNode{}, p.syntaxError("Unexpected token: " + tokenType.String())
}

func (p *Parser) nud(token Token) (ASTNode, error) {
	switch token.TokenType {
	case TJSONLiteral:
		var parsed interface{}
		err := json.Unmarshal([]byte(token.Value), &parsed)
		if err != nil {
			// NOTE: here should be error, but in legacy data this is string in case of parsing errors
			// return ASTNode{}, err
			trimmed := strings.TrimLeftFunc(token.Value, unicode.IsSpace)
			return ASTNode{NodeType: ASTLiteral, Value: trimmed}, nil
		}
		return ASTNode{NodeType: ASTLiteral, Value: parsed}, nil
	case TStringLiteral:
		return ASTNode{NodeType: ASTLiteral, Value: token.Value}, nil
	case TUnquotedIdentifier:
		return ASTNode{
			NodeType: ASTField,
			Value:    token.Value,
		}, nil
	case TQuotedIdentifier:
		node := ASTNode{NodeType: ASTField, Value: token.Value}
		if p.current() == TLparen {
			return ASTNode{}, p.syntaxErrorToken("Can't have quoted identifier as function name.", token)
		}
		return node, nil
	case TStar:
		left := ASTNode{NodeType: ASTIdentity}
		var right ASTNode
		var err error
		if p.current() == TRbracket {
			right = ASTNode{NodeType: ASTIdentity}
		} else {
			right, err = p.parseProjectionRHS(bindingPowers[TStar])
		}
		return ASTNode{NodeType: ASTValueProjection, Children: []ASTNode{left, right}}, err
	case TFilter:
		return p.parseFilter(ASTNode{NodeType: ASTIdentity})
	case TLbrace:
		return p.parseMultiSelectHash()
	case TFlatten:
		left := ASTNode{
			NodeType: ASTFlatten,
			Children: []ASTNode{{NodeType: ASTIdentity}},
		}
		right, err := p.parseProjectionRHS(bindingPowers[TFlatten])
		if err != nil {
			return ASTNode{}, err
		}
		return ASTNode{NodeType: ASTProjection, Children: []ASTNode{left, right}}, nil
	case TLbracket:
		tokenType := p.current()
		// var right ASTNode
		if tokenType == TNumber || tokenType == TColon {
			right, err := p.parseIndexExpression()
			if err != nil {
				return ASTNode{}, nil
			}
			return p.projectIfSlice(ASTNode{NodeType: ASTIdentity}, right)
		} else if tokenType == TStar && p.lookahead(1) == TRbracket {
			p.advance()
			p.advance()
			right, err := p.parseProjectionRHS(bindingPowers[TStar])
			if err != nil {
				return ASTNode{}, err
			}
			return ASTNode{
				NodeType: ASTProjection,
				Children: []ASTNode{{NodeType: ASTIdentity}, right},
			}, nil
		} else {
			return p.parseMultiSelectList()
		}
	case TCurrent:
		return ASTNode{NodeType: ASTCurrentNode}, nil
	case TExpref:
		expression, err := p.parseExpression(bindingPowers[TExpref])
		if err != nil {
			return ASTNode{}, err
		}
		return ASTNode{NodeType: ASTExpRef, Children: []ASTNode{expression}}, nil
	case TNot:
		expression, err := p.parseExpression(bindingPowers[TNot])
		if err != nil {
			return ASTNode{}, err
		}
		return ASTNode{NodeType: ASTNotExpression, Children: []ASTNode{expression}}, nil
	case TLparen:
		expression, err := p.parseExpression(0)
		if err != nil {
			return ASTNode{}, err
		}
		if err := p.match(TRparen); err != nil {
			return ASTNode{}, err
		}
		return expression, nil
	case TEOF:
		return ASTNode{}, p.syntaxErrorToken("Incomplete expression", token)
	}

	return ASTNode{}, p.syntaxErrorToken("Invalid token: "+token.TokenType.String(), token)
}

func (p *Parser) parseMultiSelectList() (ASTNode, error) {
	var expressions []ASTNode
	for {
		expression, err := p.parseExpression(0)
		if err != nil {
			return ASTNode{}, err
		}
		expressions = append(expressions, expression)
		if p.current() == TRbracket {
			break
		}
		err = p.match(TComma)
		if err != nil {
			return ASTNode{}, err
		}
	}
	err := p.match(TRbracket)
	if err != nil {
		return ASTNode{}, err
	}
	return ASTNode{
		NodeType: ASTMultiSelectList,
		Children: expressions,
	}, nil
}

func (p *Parser) parseMultiSelectHash() (ASTNode, error) {
	var children []ASTNode
	for {
		keyToken := p.lookaheadToken(0)
		if err := p.match(TUnquotedIdentifier); err != nil {
			if err := p.match(TQuotedIdentifier); err != nil {
				return ASTNode{}, p.syntaxError("Expected tQuotedIdentifier or tUnquotedIdentifier")
			}
		}
		keyName := keyToken.Value
		err := p.match(TColon)
		if err != nil {
			return ASTNode{}, err
		}
		value, err := p.parseExpression(0)
		if err != nil {
			return ASTNode{}, err
		}
		node := ASTNode{
			NodeType: ASTKeyValPair,
			Value:    keyName,
			Children: []ASTNode{value},
		}
		children = append(children, node)
		if p.current() == TComma {
			err := p.match(TComma)
			if err != nil {
				return ASTNode{}, nil
			}
		} else if p.current() == TRbrace {
			err := p.match(TRbrace)
			if err != nil {
				return ASTNode{}, nil
			}
			break
		}
	}
	return ASTNode{
		NodeType: ASTMultiSelectHash,
		Children: children,
	}, nil
}

func (p *Parser) projectIfSlice(left ASTNode, right ASTNode) (ASTNode, error) {
	indexExpr := ASTNode{
		NodeType: ASTIndexExpression,
		Children: []ASTNode{left, right},
	}
	if right.NodeType == ASTSlice {
		right, err := p.parseProjectionRHS(bindingPowers[TStar])
		return ASTNode{
			NodeType: ASTProjection,
			Children: []ASTNode{indexExpr, right},
		}, err
	}
	return indexExpr, nil
}

func (p *Parser) parseFilter(node ASTNode) (ASTNode, error) {
	var right, condition ASTNode
	var err error
	condition, err = p.parseExpression(0)
	if err != nil {
		return ASTNode{}, err
	}
	if err := p.match(TRbracket); err != nil {
		return ASTNode{}, err
	}
	if p.current() == TFlatten {
		right = ASTNode{NodeType: ASTIdentity}
	} else {
		right, err = p.parseProjectionRHS(bindingPowers[TFilter])
		if err != nil {
			return ASTNode{}, err
		}
	}

	return ASTNode{
		NodeType: ASTFilterProjection,
		Children: []ASTNode{node, right, condition},
	}, nil
}

func (p *Parser) parseDotRHS(bindingPower int) (ASTNode, error) {
	lookahead := p.current()
	if tokensOneOf([]TokType{TQuotedIdentifier, TUnquotedIdentifier, TStar}, lookahead) {
		return p.parseExpression(bindingPower)
	} else if lookahead == TLbracket {
		if err := p.match(TLbracket); err != nil {
			return ASTNode{}, err
		}
		return p.parseMultiSelectList()
	} else if lookahead == TLbrace {
		if err := p.match(TLbrace); err != nil {
			return ASTNode{}, err
		}
		return p.parseMultiSelectHash()
	}
	return ASTNode{}, p.syntaxError("Expected identifier, lbracket, or lbrace")
}

func (p *Parser) parseProjectionRHS(bindingPower int) (ASTNode, error) {
	current := p.current()
	if bindingPowers[current] < 10 {
		return ASTNode{NodeType: ASTIdentity}, nil
	} else if current == TLbracket {
		return p.parseExpression(bindingPower)
	} else if current == TFilter {
		return p.parseExpression(bindingPower)
	} else if current == TDot {
		err := p.match(TDot)
		if err != nil {
			return ASTNode{}, err
		}
		return p.parseDotRHS(bindingPower)
	} else {
		return ASTNode{}, p.syntaxError("Error")
	}
}

func (p *Parser) lookahead(number int) TokType {
	return p.lookaheadToken(number).TokenType
}

func (p *Parser) current() TokType {
	return p.lookahead(0)
}

func (p *Parser) lookaheadToken(number int) Token {
	return p.tokens[p.index+number]
}

func (p *Parser) advance() {
	p.index++
}

func tokensOneOf(elements []TokType, token TokType) bool {
	for _, elem := range elements {
		if elem == token {
			return true
		}
	}
	return false
}

func (p *Parser) syntaxError(msg string) SyntaxError {
	return SyntaxError{
		Msg:        msg,
		Expression: p.expression,
		Offset:     p.lookaheadToken(0).Position,
	}
}

// Create a SyntaxError based on the provided token.
// This differs from syntaxError() which creates a SyntaxError
// based on the current lookahead token.
func (p *Parser) syntaxErrorToken(msg string, t Token) SyntaxError {
	return SyntaxError{
		Msg:        msg,
		Expression: p.expression,
		Offset:     t.Position,
	}
}
