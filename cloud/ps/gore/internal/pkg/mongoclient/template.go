package mongoclient

import (
	"context"
	"errors"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
)

func (c *Config) StoreTemplate(sid, t, text string) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Template)
	ch := template.Template{
		ServiceID: sid,
		Type:      t,
		Text:      text,
	}
	if _, err = coll.InsertOne(ctx, ch); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) DeleteTemplate(tid string) (err error) {
	ftid, err := primitive.ObjectIDFromHex(tid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Template)
	filter := bson.M{
		"_id": ftid,
	}
	if _, err := coll.DeleteOne(ctx, filter); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) UpdateTemplate(tid string, tmp *template.Template) (err error) {
	ftid, err := primitive.ObjectIDFromHex(tid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Template)
	filter := bson.M{"_id": ftid}
	update := bson.D{
		{Key: "$set", Value: tmp},
		{Key: "$currentDate", Value: bson.M{"LastUpdated": true}},
	}
	result := coll.FindOneAndUpdate(ctx, filter, update)
	if err = result.Err(); errors.Is(err, mongo.ErrNoDocuments) || err == nil {
		return nil
	}

	c.Log.Errorf("Error: %v", err)
	return
}

func (c *Config) ListTemplates(sid, tid, t string) (res []template.Template, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	filter := bson.M{"serviceid": sid}
	if len(tid) > 0 {
		ftid, err := primitive.ObjectIDFromHex(tid)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			return nil, err
		}

		filter["_id"] = ftid
	}

	if len(t) > 0 {
		filter["type"] = t
	}

	coll := c.db.Collection(c.Collections.Template)
	cur, err := coll.Find(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	defer cur.Close(context.Background())

	for cur.Next(context.Background()) {
		single := new(template.Template)
		if err := cur.Decode(single); err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		res = append(res, *single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Templates cannot be retrieved: %v", err)
	}

	return
}

func (c *Config) DeleteTemplatesByIDs(ids []string) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	oids := []primitive.ObjectID{}
	for _, id := range ids {
		oid, err := primitive.ObjectIDFromHex(id)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		oids = append(oids, oid)
	}

	coll := c.db.Collection(c.Collections.Template)
	filter := bson.M{"_id": bson.M{"$in": oids}}
	_, err = coll.DeleteMany(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) CreateTemplates(ts []template.Template) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Template)
	ins := []interface{}{}
	for _, t := range ts {
		ins = append(ins, t)
	}

	_, err = coll.InsertMany(ctx, ins)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) GetActiveTemplate(sid, t string) (res *template.Template, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	filter := bson.M{
		"serviceid": sid,
		"active":    true,
		"type":      t,
	}
	coll := c.db.Collection(c.Collections.Template)
	sr := coll.FindOne(ctx, filter)
	res = new(template.Template)
	if err = sr.Decode(res); errors.Is(err, mongo.ErrNoDocuments) || err == nil {
		return
	}

	c.Log.Warnf("Error: %v", err)
	return
}

func (c *Config) CheckForTemplateConflicts(ts []template.Template) (res []template.Template, err error) {
	fts := bson.A{}
	for _, t := range ts {
		if t.Unique {
			fts = append(fts, bson.M{
				"serviceid": t.ServiceID,
				"type":      t.Type,
			})
		}
	}

	if len(fts) < 1 {
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Template)
	filter := bson.M{"$or": fts}
	cur, err := coll.Find(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	defer cur.Close(context.Background())

	for cur.Next(context.Background()) {
		single := new(template.Template)
		if err := cur.Decode(single); err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		res = append(res, *single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Templates cannot be retrieved: %v", err)
	}

	return
}
