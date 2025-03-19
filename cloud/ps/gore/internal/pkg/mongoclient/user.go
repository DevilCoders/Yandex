package mongoclient

// import (
// 	"context"
// 	"errors"
// 	"time"

// 	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/user"
// 	"go.mongodb.org/mongo-driver/bson"
// 	"go.mongodb.org/mongo-driver/bson/primitive"
// 	"go.mongodb.org/mongo-driver/mongo"
// )

// func (c *Config) StoreUser(sid, u, text string) (err error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	coll := c.db.Collection(c.Collections.User)
// 	ch := user.User{
// 		ServiceID: sid,
// 		Type:      t,
// 		Text:      text,
// 	}
// 	if _, err = coll.InsertOne(ctx, ch); err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 	}

// 	return
// }

// func (c *Config) DeleteUser(tid string) (err error) {
// 	ftid, err := primitive.ObjectIDFromHex(tid)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 		return
// 	}

// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	coll := c.db.Collection(c.Collections.User)
// 	filter := bson.M{
// 		"_id": ftid,
// 	}
// 	if _, err := coll.DeleteOne(ctx, filter); err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 	}

// 	return
// }

// func (c *Config) UpdateUser(tid string, tmp *template.User) (err error) {
// 	ftid, err := primitive.ObjectIDFromHex(tid)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 		return
// 	}

// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	coll := c.db.Collection(c.Collections.User)
// 	filter := bson.M{"_id": ftid}
// 	update := bson.D{
// 		{Key: "$set", Value: tmp},
// 		{Key: "$currentDate", Value: bson.M{"LastUpdated": true}},
// 	}
// 	result := coll.FindOneAndUpdate(ctx, filter, update)
// 	if err = result.Err(); err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 		return
// 	}

// 	return
// }

// func (c *Config) ListUsers(sid, tid, t string) (res []template.User, err error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	filter := bson.M{"serviceid": sid}
// 	if len(tid) > 0 {
// 		filter["_id"] = tid
// 	}

// 	if len(t) > 0 {
// 		filter["type"] = t
// 	}

// 	coll := c.db.Collection(c.Collections.User)
// 	cur, err := coll.Find(ctx, filter)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 		return
// 	}

// 	defer cur.Close(context.Background())

// 	for cur.Next(context.Background()) {
// 		single := new(template.User)
// 		if err := cur.Decode(single); err != nil {
// 			c.Log.Errorf("Error: %v", err)
// 			continue
// 		}
// 		res = append(res, *single)
// 	}

// 	if err = cur.Err(); err != nil {
// 		c.Log.Errorf("Users cannot be retrieved: %v", err)
// 	}

// 	return
// }

// func (c *Config) DeleteUsersByIDs(ids []string) (err error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	oids := []primitive.ObjectID{}
// 	for _, id := range ids {
// 		oid, err := primitive.ObjectIDFromHex(id)
// 		if err != nil {
// 			c.Log.Errorf("Error: %v", err)
// 			continue
// 		}
// 		oids = append(oids, oid)
// 	}

// 	coll := c.db.Collection(c.Collections.User)
// 	filter := bson.M{"_id": bson.M{"$in": oids}}
// 	_, err = coll.DeleteMany(ctx, filter)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 	}

// 	return
// }

// func (c *Config) CreateUsers(ts []template.User) (err error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	coll := c.db.Collection(c.Collections.User)
// 	ins := []interface{}{}
// 	for _, t := range ts {
// 		ins = append(ins, t)
// 	}

// 	_, err = coll.InsertMany(ctx, ins)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 	}

// 	return
// }

// func (c *Config) GetActiveUser(sid, t string) (res *template.User, err error) {
// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	filter := bson.M{
// 		"serviceid": sid,
// 		"active":    true,
// 		"type":      t,
// 	}
// 	coll := c.db.Collection(c.Collections.User)
// 	sr := coll.FindOne(ctx, filter)
// 	res = new(template.User)
// 	if err = sr.Decode(res); errors.Is(err, mongo.ErrNoDocuments) || err == nil {
// 		return
// 	}

// 	c.Log.Warnf("Error: %v", err)
// 	return
// }

// func (c *Config) CheckForUserConflicts(ts []template.User) (res []template.User, err error) {
// 	fts := bson.A{}
// 	for _, t := range ts {
// 		if t.Unique {
// 			fts = append(fts, bson.M{
// 				"serviceid": t.ServiceID,
// 				"type":      t.Type,
// 			})
// 		}
// 	}

// 	if len(fts) < 1 {
// 		return
// 	}

// 	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
// 	defer cancel()

// 	coll := c.db.Collection(c.Collections.User)
// 	filter := bson.M{"$or": fts}
// 	cur, err := coll.Find(ctx, filter)
// 	if err != nil {
// 		c.Log.Errorf("Error: %v", err)
// 		return
// 	}

// 	defer cur.Close(context.Background())

// 	for cur.Next(context.Background()) {
// 		single := new(template.User)
// 		if err := cur.Decode(single); err != nil {
// 			c.Log.Errorf("Error: %v", err)
// 			continue
// 		}
// 		res = append(res, *single)
// 	}

// 	if err = cur.Err(); err != nil {
// 		c.Log.Errorf("Users cannot be retrieved: %v", err)
// 	}

// 	return
// }
