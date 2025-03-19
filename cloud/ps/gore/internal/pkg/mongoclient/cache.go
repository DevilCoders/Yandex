package mongoclient

import (
	"context"
	"fmt"
	"time"

	"go.mongodb.org/mongo-driver/bson"
)

type Cache struct {
	Key     string
	Content []byte
	Expire  int64
}

func (c *Config) CacheStore(key string, content []byte, duration time.Duration) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Cache)
	ch := Cache{
		Key:     key,
		Content: content,
		Expire:  time.Now().Add(duration).UnixNano(),
	}

	_, err = coll.InsertOne(ctx, ch)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) CacheGet(key string) (content []byte, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Cache)
	var ch Cache

	filter := bson.M{"key": key}
	s := coll.FindOne(ctx, filter)
	err = s.Decode(&ch)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	if ch.Expire < time.Now().UnixNano() {
		content = ch.Content
	} else {
		err = fmt.Errorf("content expired")
		c.Log.Errorf("Error: %v", err)
	}
	return
}
