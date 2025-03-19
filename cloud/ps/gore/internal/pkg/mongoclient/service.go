package mongoclient

import (
	"context"
	"errors"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/service"
	"github.com/imdario/mergo"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

func (c *Config) GetServiceByName(sn string, fields []string) (result *service.Service, err error) {
	coll := c.db.Collection(c.Collections.Services)
	p := make(map[string]int)
	if len(fields) > 0 {
		for _, f := range fields {
			p[f] = 1
		}
	}

	filter := bson.M{"service": sn}
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	s := coll.FindOne(ctx, filter, options.FindOne().SetProjection(p))
	result = new(service.Service)
	if err = s.Decode(result); errors.Is(err, mongo.ErrNoDocuments) || err == nil {
		return
	}

	c.Log.Warnf("Error: %v", err)
	return
}

func (c *Config) GetServiceByID(sid string, fields []string) (result *service.Service, err error) {
	coll := c.db.Collection(c.Collections.Services)
	fsid, err := primitive.ObjectIDFromHex(sid)
	if err != nil {
		c.Log.Warnf("Error: %v", err)
		return
	}

	p := make(map[string]int)
	if len(fields) > 0 {
		for _, f := range fields {
			p[f] = 1
		}
	}
	filter := bson.M{"_id": fsid}
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	s := coll.FindOne(ctx, filter, options.FindOne().SetProjection(p))

	result = new(service.Service)
	if err = s.Decode(result); errors.Is(err, mongo.ErrNoDocuments) || err == nil {
		return
	}

	c.Log.Warnf("Error: %v", err)
	return
}

func (c *Config) GetServicesAll(fields []string) (result []*service.Service, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	p := make(map[string]int)
	if len(fields) > 0 {
		for _, f := range fields {
			p[f] = 1
		}
	}

	cur, err := coll.Find(ctx, bson.M{}, options.Find().SetProjection(p))
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}
	defer cur.Close(context.Background())

	for cur.Next(context.Background()) {
		single := new(service.Service)
		err := cur.Decode(single)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		result = append(result, single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Services cannot be retrieved: %v", err)
	}

	return
}

func (c *Config) CreateService(s *service.Service) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	_, err = coll.InsertOne(ctx, s)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) DeleteServiceByName(sn string) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	filter := bson.M{"service": sn}
	_, err = coll.DeleteOne(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) DeleteServiceByID(sid string) (err error) {
	fsid, err := primitive.ObjectIDFromHex(sid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	filter := bson.M{"_id": fsid}
	_, err = coll.DeleteOne(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) UpdateServiceByName(sn string, s *service.Service) (err error) {
	sd, err := c.GetServiceByName(sn, nil)
	if err != nil {
		return
	}

	if err = mergo.Merge(sd, s, mergo.WithOverride); err != nil {
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	filter := bson.M{"service": sn}
	update := bson.D{
		{Key: "$set", Value: sd},
		{Key: "$currentDate", Value: bson.M{"LastUpdated": true}},
	}
	result := coll.FindOneAndUpdate(ctx, filter, update)
	err = result.Err()
	if err != nil {
		c.Log.Errorf("Service %s cannot be updated: %v", s.Service, err)
	}

	return
}

func (c *Config) UpdateServiceByID(sid string, s *service.Service) (err error) {
	fsid, err := primitive.ObjectIDFromHex(sid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	sd, err := c.GetServiceByID(sid, nil)
	if err != nil {
		return
	}

	if err = mergo.Merge(sd, s, mergo.WithOverride); err != nil {
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Services)
	filter := bson.M{"_id": fsid}
	update := bson.D{
		{Key: "$set", Value: sd},
		{Key: "$currentDate", Value: bson.M{"LastUpdated": true}},
	}

	result := coll.FindOneAndUpdate(ctx, filter, update)
	err = result.Err()
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}
