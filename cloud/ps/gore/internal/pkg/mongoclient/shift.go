package mongoclient

import (
	"context"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/shift"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
)

func (c *Config) RenewShiftsForService(shs []shift.Shift) (err error) {
	if len(shs) < 1 {
		return
	}

	if err = c.DeleteShiftsForService(shs[0].ServiceID); err != nil {
		return
	}

	return c.StoreShifts(shs)
}

func (c *Config) StoreShifts(shs []shift.Shift) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	p := make([]interface{}, len(shs))
	for i, v := range shs {
		p[i] = v
	}

	coll := c.db.Collection(c.Collections.Shifts)
	if _, err = coll.InsertMany(ctx, p); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

// func GetUserShiftsForDate(date int64, sid string) (shs []shift.Shift, err error) {
// 	return GetShifts(date, sid, true)
// }

// func GetRobotShiftsForDate(date int64, sid string) (shs []shift.Shift, err error) {
// 	return GetShifts(date, sid, false)
// }

// func GetUserShifts(sid string) (shs []shift.Shift, err error) {
// 	return GetShifts(0, sid, true)
// }

// func GetRobotShifts(sid string) (shs []shift.Shift, err error) {
// 	return GetShifts(0, sid, false)
// }

// func GetUserShiftsForDateAll(date int64) (shs []shift.Shift, err error) {
// 	return GetShifts(date, "", true)
// }

// func GetRobotShiftsForDateAll(date int64) (shs []shift.Shift, err error) {
// 	return GetShifts(date, "", false)
// }

func (c *Config) GetShifts(date int64, sid string, user bool) (shs []shift.Shift, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{}
	if date > 0 {
		filter["datestart"] = bson.M{"$lt": date}
		filter["dateend"] = bson.M{"$gt": date}
	}

	if len(sid) > 0 {
		filter["serviceid"] = sid
	}

	if user {
		filter["resp.order"] = bson.M{"$gte": 0}
	}

	cur, err := coll.Find(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	var single shift.Shift
	for cur.Next(context.Background()) {
		err := cur.Decode(&single)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		shs = append(shs, single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	return
}

// GetShiftsForInterval returns shifts stored in MongoDB.
// start, end - timestamp; sid - service ID; login - staff login; user - real users (order > 0) or all
func (c *Config) GetShiftsForInterval(start, end int64, sid, login string, user bool) (shs []shift.Shift, err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{}
	if end > 0 {
		filter["datestart"] = bson.M{"$lte": end}
	}

	if start > 0 {
		filter["dateend"] = bson.M{"$gte": start}
	}

	if len(sid) > 0 {
		filter["serviceid"] = sid
	}

	if user {
		filter["resp.order"] = bson.M{"$gte": 0}
	}

	if len(login) > 0 {
		filter["resp.username"] = login
	}

	cur, err := coll.Find(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	var single shift.Shift
	for cur.Next(context.Background()) {
		err := cur.Decode(&single)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		shs = append(shs, single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	return
}

func (c *Config) DeleteShiftsForService(sid string) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"serviceid": sid}
	if _, err = coll.DeleteMany(ctx, filter); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) UpdateShiftByID(shid string, sh *shift.Shift) (err error) {
	fsid, err := primitive.ObjectIDFromHex(shid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"_id": fsid}
	update := bson.D{
		{Key: "$set", Value: sh},
	}
	result := coll.FindOneAndUpdate(ctx, filter, update)
	if err = result.Err(); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) DeleteShiftByID(shid string, sh *shift.Shift) (err error) {
	fsid, err := primitive.ObjectIDFromHex(shid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"_id": fsid}
	if _, err = coll.DeleteOne(ctx, filter); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) CreateShifts(shs []shift.Shift) (err error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	ins := []interface{}{}
	for _, sh := range shs {
		ins = append(ins, sh)
	}

	if _, err = coll.InsertMany(ctx, ins); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) GetShiftsByIDs(ids []string) (shs []shift.Shift, err error) {
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

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"_id": bson.M{"$in": oids}}
	cur, err := coll.Find(ctx, filter)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	single := new(shift.Shift)
	for cur.Next(context.Background()) {
		err := cur.Decode(single)
		if err != nil {
			c.Log.Errorf("Error: %v", err)
			continue
		}
		shs = append(shs, *single)
	}

	if err = cur.Err(); err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	return
}

func (c *Config) DeleteShiftsByIDs(ids []string) (err error) {
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

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"_id": bson.M{"$in": oids}}
	if _, err = coll.DeleteMany(ctx, filter); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}

func (c *Config) GetShiftForServiceByID(sid, shid string) (sh *shift.Shift, err error) {
	fsid, err := primitive.ObjectIDFromHex(shid)
	if err != nil {
		c.Log.Errorf("Error: %v", err)
		return
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	coll := c.db.Collection(c.Collections.Shifts)
	filter := bson.M{"_id": fsid, "serviceid": sid}
	cur := coll.FindOne(ctx, filter)
	sh = new(shift.Shift)
	if err := cur.Decode(sh); err != nil {
		c.Log.Errorf("Error: %v", err)
	}

	return
}
