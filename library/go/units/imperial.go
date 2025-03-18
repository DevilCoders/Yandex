package units

// Length units
const (
	Thou    = 0.0000254 * Meter
	Inch    = 1000 * Thou
	Foot    = 12 * Inch
	Yard    = 3 * Foot
	Chain   = 22 * Yard
	Furlong = 10 * Chain
	Mile    = 8 * Furlong

	// deprecated units
	League = 3 * Mile
	Link   = 7.92 * Inch
	Rod    = 25 * Link
)

// Area units
const (
	Perch = Rod * Rod
	Rood  = Furlong * Rod
	Acre  = Furlong * Chain
)

// Volume units
const (
	Ounce  = 28.4130625 * Milliliter
	Gill   = 5 * Ounce
	Pint   = 20 * Ounce
	Quart  = 40 * Ounce
	Gallon = 160 * Ounce
)

// Mass units
const (
	Grain         = 64.79891 * Milligram
	Drachm        = Pound * (float32(1) / 256)
	Pound         = 0.45359237 * Kilogram
	Stone         = 14 * Pound
	Quarter       = 28 * Pound
	Hundredweight = 112 * Pound
	Ton           = 2240 * Pound
)

// Gravitational units
const (
	Slug = 32.17404856 * Pound
)
