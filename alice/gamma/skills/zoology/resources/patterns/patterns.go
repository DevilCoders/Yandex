package patterns

import (
	"fmt"

	sdk "a.yandex-team.ru/alice/gamma/sdk/golang"
)

const (
	StartGameIntent        = "start"
	RulesIntent            = "rules"
	RestartGameIntent      = "restart"
	HowDoesAliceKnowIntent = "how"
	ResultIntent           = "result"
	RepeatQuestionIntent   = "repeat"
	EndGameIntent          = "end"

	YesIntent = "yes"
	NoIntent  = "no"

	AnswerIntent          = "answer"
	InvalidAnswerIntent   = "invalid"
	UserDoesntKnowIntent  = "dont_know"
	NegativeAnswerIntent  = "negative"
	UncertainAnswerIntent = "uncertain"
	UndefinedAnswerIntent = "undefined"
	NextQuestionIntent    = "next"
	NoContinueIntent      = "dont"
	YesContinueIntent     = "yes"
)

const (
	agreeStrong = "(давай|конечно|конешно|канешна|а то [нет]|очень|[ты] прав*|абсолютно|обязательно|непременно|а как же|" +
		"подтверждаю|точно|пожалуй|запросто|норм|(почему|что|че) [бы] [и] нет|хочу|было [бы] (неплохо|не плохо)|" +
		"[([ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|окай|okay|оке|именно|подтвержд*|йес) " +
		"[да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно]|очень)] (хочу|хо чу|ладно|хорошо|хoрoшo|можно|валяй*|договорились|" +
		"согла*|вполне|в полной мере|естественно|разумеется|(еще|ещё) как|не (против|возражаю|сомневаюсь)|" +
		"я только за|безусловн*|[это] так [и есть]|давай*) [(конечно|конешно|канешна)]|[все] правильно|все так)"
	you = "[(у|про|для|за|из [за]|из-за|в|без|до|через|с|со|об|от|к|ко|о|обо|об|при|по|на|с|над|под|перед)]" +
		" (ты|тебя|тибя|тя|тебе|тобой|вы|вас|вам|вами|твой|твоя|твою|твое*|твои*|ваш*|" +
		"свое*|своё|свои*|свой|свою|своя)"
	stopGame = "(* (давай (хватит|закончим)|хватит играть|надоело|(больше не (хочу|хочется))|" +
		"прекращ*|прекрат*|стоп|останов*|заканчивай|заканчиваю|[давай] (закончим|закончи|закончить) [игр*]|устал* игр*|" +
		"(не (хочу|хочется) [больше] играть)|надоел|надоела|мне надоело играть|" +
		"((не нравится) * игра)|(выйти|выход) из * игры)*|" +
		"(конец|хватит|[я] устал*|выход|выйти|сдаюсь))"
	agree = "(* (давай|конечно|конешно|канешна|а то [нет]|очень|[ты] прав*|абсолютно|обязательно|непременно|а как же|" +
		"подтверждаю|точно|пожалуй|запросто|норм|(почему|что|че) [бы] [и] нет|хочу|было [бы] (неплохо|не плохо)|" +
		"[([ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|окай|okay|оке|именно|" +
		"подтвержд*|йес) [да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно]|очень)] (хочу|хо чу|ладно|хорошо|" +
		"хoрoшo|можно|валяй*|договорились|согла*|вполне|в полной мере|" +
		"естественно|разумеется|(еще|ещё) как|не (против|возражаю|сомневаюсь)|я только за|безусловн*|" +
		"[это] так [и есть]|давай*) [(конечно|конешно|канешна)]|[все] правильно|все так) * |(логично|могу|было дело|" +
		"бывало|бывает)|[ну] [конечно|всё|все|вроде|пожалуй|возможно] (да|даа|lf|ага|точно|угу|верно|ок|ok|окей|" +
		"окай|okay|оке|именно|подтвержд*|йес) [да|конечно|конешно|канешна|всё|все|вроде|пожалуй|возможно])"
	startGame = "(начин*|начн*|попытаюсь|попробую|поехали|го|гоу|погнали|продолж*|готов*|давай|валяй|реди|рэди|камон|" +
		"сыграем|сыграю|играем|играю)"
	disagree = "(* [((нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо]|конечно|конешно|канешна)] " +
		"((нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо]|не сейчас|ни капли|отнюдь|нискол*|да ладно|не" +
		" (хоч*|хо чу|надо|могу|очень|думаю|нравится|стоит|буду|считаю|согла*|подтв*)|ненадо|нельзя|нехочу|ненавижу|" +
		"невозможно|никогда|никуда|ни за что|нисколько|никак*|никто|ниразу|[я] против|вряд ли|сомневаюсь|нихрена|" +
		"неправильно|неверно|невсегда|[это] (не так)|отказываюсь) [(конечно|конешно|канешна|спасибо)]" +
		" *|(да ну|не|нее|ничего)|(нет|неат|ниат|неа|ноуп|ноу|найн) [нет] [спасибо])"
	notNow = "(не [могу] сейчас|мне (некогда|не до *того)|(в другой|не в этот) раз|[давай] (*позже|потом|не сегодня|" +
		"не сейчас|пока не надо) [поговорим]|нет времени|я (занят|занята))"
	maybe       = "((может [быть]|наверн*|возможн*|вероятн*|скорее всего|вроде [бы|да]) [тогда])"
	sure        = "(конечно|канешна|конешно|безусловно|очевидно|логично|точно) [же]"
	quizAll     = "($Animal|$Order|$Class|$Savagery|$NUMBER|$Coating|$Extremities)"
	preposition = "(у|про|для|за|из [за]|из-за|в|без|до|через|с|со|об|от|к|ко|о|обо|об|при|по|на|с|над|под|перед)"
	dontKnow    = "[([я] даже|ну [я]|сам*)] ((не|откуда [(у|про|для|за|из [за]|из-за|в|без|до|через|с|со|об|от|к|ко|о|обо|" +
		"об|при|по|на|с|над|под|перед)] (я|меня|мне|мной|мой|моя|мою|мое*|моим|моём|моей|себ*|мня|мну)) (зна*|задумывал*|" +
		"думал*|решил*|представляю|уверен*|помню)|" +
		"незнаю|неизвестно|надо подумать|без понятия|без понятий|понятия не имею|сомневаюсь|сложны* вопрос*|" +
		"|сложно сказать|все сложно|ну ты [и]спросил|спросишь тоже|забыл*|сдаюсь)"
)

var GlobalCommands = []sdk.Pattern{
	{
		Name: RestartGameIntent,
		Pattern: fmt.Sprintf("* ( [%s] * (еще|ещё|продолж*|повторим) [раз] [сыгр*|игр*|поигр*] [в (зоо*|животных)] ) *|"+
			"* ((еще|ещё|продолж*|повторим) [раз] (сыгр*|игр*|поигр*)) *|"+
			"* ( [%s] * (еще|ещё) вопрос* ) *|"+
			"* ( [%s] * (еще|ещё|продолж*|повторим) [раз] [сыгр*|игр*|поигр*] [в (зоо*|животных)] ) *|"+
			"* ((еще|ещё|продолж*|повторим) [раз] (сыгр*|игр*|поигр*)) *|"+
			"* ( [%s] * (еще|ещё) вопрос* ) *", agreeStrong, agreeStrong, agreeStrong, agreeStrong),
	},
	{
		Name: StartGameIntent,
		Pattern: "* ([давай|может] * (сыграем|игра*|поигра*|игру|сыгра*|включи|активируй|начать|начнем|начина*) * [в] (зоо*|животн*|звер*)) *|" +
			"* ((давай|хочу|хочется|включи|активируй|начать|начнем|начина*) * (зоо*|животн*|звер*)) *",
	},
	{
		Name: RulesIntent,
		Pattern: "* [еще] [раз] (правила|как играть|что делать|как дальше) *|" +
			"(и че|и что|и как)",
	},
	{
		Name:    HowDoesAliceKnowIntent,
		Pattern: fmt.Sprintf("* (%s * откуда * знаешь) *", you),
	},
	{
		Name:    ResultIntent,
		Pattern: "* (скол* * правильн* * ответ*) *",
	},
	{
		Name: EndGameIntent,
		Pattern: fmt.Sprintf("* %s *|"+
			"* [давай] * перерыв *|"+
			"[я] устал*", stopGame),
	},
}

var SelectCommands = []sdk.Pattern{
	{
		Name: YesIntent,
		Pattern: fmt.Sprintf("* (%s|%s|начин*|начн*|помогу|стараться|попытаюсь|попробую|давай) "+
			"[начин*|начн*|поигра*|сыгра*|игра*|попробую|попробуем] *", agree, startGame),
	},
	{
		Name:    NoIntent,
		Pattern: fmt.Sprintf("* (%s|%s) *", disagree, notNow),
	},
}

var QuestionCommands = []sdk.Pattern{
	{
		Name:    NegativeAnswerIntent,
		Pattern: fmt.Sprintf("* [%s] не [%s] %s *", sure, preposition, quizAll),
	},
	{
		Name: UserDoesntKnowIntent,
		Pattern: fmt.Sprintf("* %s  * %s или %s *|"+
			"* %s или %s * %s *|"+
			"* %s *|"+
			"у кого", dontKnow, quizAll, quizAll, quizAll, quizAll, dontKnow, dontKnow),
	},
	{
		Name: InvalidAnswerIntent,
		Pattern: fmt.Sprintf("* ([ни|или|и] [у] (то*|та|те) [ни|или|и] [у] (то*|та|те|друг*)|ничего [нет]) *|"+
			"* ([ни|или|и] [у] (то*|та|те) [ни|или|и] [у] (то*|та|те|друг*)|ничего [нет]) *|"+
			"* ([ни|нет] [у|из] (них|обоих|кого)|оба|обе|никто) *|"+
			"* [%s|%s] (($Animal * $Animal)|($Order * $Order)|($Coating * $Coating)|($Parts * $Parts)|($Class * $Class)|($Savagery * $Savagery)|"+
			"($NUMBER * $NUMBER)) [%s|%s] *|"+
			"* [%s|%s] * ($Order * $Order) [%s|%s] *", maybe, sure, maybe, sure, maybe, sure, maybe, sure),
	},
	{
		Name:    AnswerIntent,
		Pattern: fmt.Sprintf("* [%s|%s] * %s * [%s|%s] *", maybe, sure, quizAll, maybe, sure),
	},
	{
		Name:    UncertainAnswerIntent,
		Pattern: "* (много|мало|нисколько) *",
	},
	{
		Name: NoContinueIntent,
		Pattern: fmt.Sprintf("* ((неправильный|плохой) * вопрос) *|"+
			"* ( [%s] [следующ*|друг*|еще|ещё] вопрос* ) *|"+
			"* ( [%s] (следующ*|друг*|дальше|еще|ещё|продолжай*|продолж*) [вопрос*] ) *", agree, agree),
	},
	{
		Name: YesContinueIntent,
		Pattern: fmt.Sprintf("* (%s|давай) *|"+
			"* %s *", agree, disagree),
	},
	{
		Name:    UndefinedAnswerIntent,
		Pattern: fmt.Sprintf("(%s|%s)", agree, disagree),
	},
}
