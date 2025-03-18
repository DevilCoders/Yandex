package compress

import (
	"bytes"
	"compress/gzip"
	"fmt"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/google/go-cmp/cmp"
	"github.com/google/go-cmp/cmp/cmpopts"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestGzipCompressor(t *testing.T) {
	t.Run("uncompressed", func(t *testing.T) {
		w := httptest.NewRecorder()

		c := NewGzipCompressor(5)
		c.Setup(w)

		payload := []byte("pong")
		written, err := c.Write(payload)
		require.NoError(t, err)
		assert.Equal(t, 4, written)

		c.Header().Set("Content-Type", "text/plain")

		err = c.Close()
		require.NoError(t, err)

		assert.Equal(t, "text/plain", w.Header().Get("Content-Type"))
		assert.Empty(t, w.Header().Get("Content-Encoding"))
		assert.Equal(t, "4", w.Header().Get("Content-Length"))
		assert.Equal(t, payload, w.Body.Bytes())
	})

	t.Run("compressed_chunks", func(t *testing.T) {
		w := httptest.NewRecorder()

		c := NewGzipCompressor(5)
		c.Setup(w)

		// make
		payload := []byte(`[
			  {"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO","email":"juanitataylor@vortexaco.com","phone":"+1 (975) 500-3334","address":"371 Middleton Street, Walland, New Hampshire, 326","about":"Exercitation dolore excepteur quis ad Lorem irure velit nisi nostrud pariatur. Dolor elit esse voluptate sunt est culpa. Ea dolor velit irure do laborum consequat. Excepteur aliqua anim elit proident ad enim ea ea dolor eiusmod voluptate.\r\n","registered":"2014-03-14T03:48:21 -04:00","latitude":-18.303385,"longitude":169.562065,"tags":["culpa","est","do","ea","ullamco","pariatur","do"],"friends":[{"id":0,"name":"Lily Ramsey"},{"id":1,"name":"Christensen Hahn"},{"id":2,"name":"Berta Murray"}],"greeting":"Hello, Juanita Taylor! You have 9 unread messages.","favoriteFruit":"banana"},
			  {"_id":"5f995e91892fc5c887bf75f1","index":1,"guid":"89addd5e-12d6-4ebb-bf48-1da40437febf","isActive":false,"balance":"$3,288.49","picture":"http://placehold.it/32x32","age":24,"eyeColor":"blue","name":"Prince Mcdowell","gender":"male","company":"ZAJ","email":"princemcdowell@zaj.com","phone":"+1 (805) 582-3382","address":"976 Ingraham Street, Ironton, Alaska, 3932","about":"Eiusmod laborum id in et excepteur mollit voluptate enim enim. Aliqua nisi aliqua ullamco velit sint non proident aute quis reprehenderit ea. Aute velit consequat qui est. Mollit ipsum eu cupidatat esse laboris eiusmod officia quis quis nisi fugiat sint ut. Sint voluptate nisi sunt proident elit irure.\r\n","registered":"2019-01-26T06:17:58 -03:00","latitude":72.746999,"longitude":-129.449557,"tags":["Lorem","non","dolor","velit","tempor","do","eu"],"friends":[{"id":0,"name":"Nunez Luna"},{"id":1,"name":"Roman Aguilar"},{"id":2,"name":"Fischer Phelps"}],"greeting":"Hello, Prince Mcdowell! You have 4 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e913235fa8186b9cb56","index":2,"guid":"645c6c52-d527-49d9-b32e-6d23ed8448ce","isActive":false,"balance":"$1,192.59","picture":"http://placehold.it/32x32","age":35,"eyeColor":"brown","name":"Sandoval Barr","gender":"male","company":"QUORDATE","email":"sandovalbarr@quordate.com","phone":"+1 (883) 499-2823","address":"120 Village Road, Ribera, Vermont, 8987","about":"Proident aliquip nisi magna ipsum velit ex ut qui adipisicing ullamco. Sint aute consectetur aute occaecat minim aliquip in aliquip. Lorem sunt elit dolore ad laboris laboris aliquip anim id duis veniam. Reprehenderit labore ad nulla esse elit reprehenderit officia dolore Lorem. Voluptate occaecat est quis adipisicing dolor deserunt nostrud sunt magna sit.\r\n","registered":"2018-05-23T01:03:13 -03:00","latitude":9.736122,"longitude":-95.864018,"tags":["veniam","do","do","irure","aliqua","mollit","dolor"],"friends":[{"id":0,"name":"Kate Chavez"},{"id":1,"name":"Ratliff Huff"},{"id":2,"name":"Francine Buchanan"}],"greeting":"Hello, Sandoval Barr! You have 1 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e91104554540c61dcb2","index":3,"guid":"37daf65f-0992-49e0-9761-4ab2d233e78f","isActive":true,"balance":"$1,827.81","picture":"http://placehold.it/32x32","age":31,"eyeColor":"blue","name":"Holman Burnett","gender":"male","company":"CORECOM","email":"holmanburnett@corecom.com","phone":"+1 (882) 568-2442","address":"538 Everit Street, Strykersville, North Carolina, 5848","about":"Est incididunt velit quis dolor incididunt excepteur sit eu ex id ipsum consequat elit minim. Tempor id dolore voluptate consequat voluptate et commodo culpa dolor. Minim amet officia enim laborum exercitation aliqua qui enim dolor magna sint cupidatat quis. Magna incididunt consequat laborum dolor incididunt fugiat ea. Culpa occaecat anim ea sint consequat Lorem labore sint. Exercitation Lorem sunt aliqua aliqua ut.\r\n","registered":"2019-03-16T11:06:38 -03:00","latitude":-89.051396,"longitude":-60.91399,"tags":["culpa","ut","mollit","adipisicing","nostrud","aliqua","nulla"],"friends":[{"id":0,"name":"Rowe Goodman"},{"id":1,"name":"Romero Sharpe"},{"id":2,"name":"Nina Justice"}],"greeting":"Hello, Holman Burnett! You have 1 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e91d03c1544744b25ec","index":4,"guid":"3bb0e185-3e62-4f42-982f-886b09cc777b","isActive":true,"balance":"$2,029.32","picture":"http://placehold.it/32x32","age":23,"eyeColor":"blue","name":"Acevedo Finch","gender":"male","company":"SPRINGBEE","email":"acevedofinch@springbee.com","phone":"+1 (963) 465-2586","address":"308 Brooklyn Avenue, Baker, Utah, 3673","about":"Lorem consectetur velit excepteur ad laboris non proident excepteur Lorem. Et anim deserunt nisi eiusmod mollit amet et velit fugiat. Minim ad aliquip anim aute.\r\n","registered":"2018-05-17T03:22:44 -03:00","latitude":-17.841303,"longitude":43.925038,"tags":["consequat","cillum","consequat","ipsum","pariatur","aliqua","sunt"],"friends":[{"id":0,"name":"Lauren Clarke"},{"id":1,"name":"Hewitt Bean"},{"id":2,"name":"Kirby Morse"}],"greeting":"Hello, Acevedo Finch! You have 6 unread messages.","favoriteFruit":"banana"}
			]
 		`)

		for _, b := range payload {
			written, err := c.Write([]byte{b})
			require.NoError(t, err)
			assert.Equal(t, 1, written)
		}

		c.Header().Set("Content-Type", "text/plain")

		err := c.Close()
		require.NoError(t, err)

		assert.Equal(t, "text/plain", w.Header().Get("Content-Type"))
		assert.Equal(t, "gzip", w.Header().Get("Content-Encoding"))
		assert.Equal(t, "2123", w.Header().Get("Content-Length"))
		assert.NotEqual(t, payload, w.Body.Bytes())

		// decode body
		cr, err := gzip.NewReader(w.Body)
		require.NoError(t, err)

		var r bytes.Buffer
		_, err = r.ReadFrom(cr)
		require.NoError(t, err)
		assert.Equal(t, payload, r.Bytes())
	})

	t.Run("compressed", func(t *testing.T) {
		w := httptest.NewRecorder()

		c := NewGzipCompressor(5)
		c.Setup(w)

		// make
		payload := []byte(`[
			  {"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO","email":"juanitataylor@vortexaco.com","phone":"+1 (975) 500-3334","address":"371 Middleton Street, Walland, New Hampshire, 326","about":"Exercitation dolore excepteur quis ad Lorem irure velit nisi nostrud pariatur. Dolor elit esse voluptate sunt est culpa. Ea dolor velit irure do laborum consequat. Excepteur aliqua anim elit proident ad enim ea ea dolor eiusmod voluptate.\r\n","registered":"2014-03-14T03:48:21 -04:00","latitude":-18.303385,"longitude":169.562065,"tags":["culpa","est","do","ea","ullamco","pariatur","do"],"friends":[{"id":0,"name":"Lily Ramsey"},{"id":1,"name":"Christensen Hahn"},{"id":2,"name":"Berta Murray"}],"greeting":"Hello, Juanita Taylor! You have 9 unread messages.","favoriteFruit":"banana"},
			  {"_id":"5f995e91892fc5c887bf75f1","index":1,"guid":"89addd5e-12d6-4ebb-bf48-1da40437febf","isActive":false,"balance":"$3,288.49","picture":"http://placehold.it/32x32","age":24,"eyeColor":"blue","name":"Prince Mcdowell","gender":"male","company":"ZAJ","email":"princemcdowell@zaj.com","phone":"+1 (805) 582-3382","address":"976 Ingraham Street, Ironton, Alaska, 3932","about":"Eiusmod laborum id in et excepteur mollit voluptate enim enim. Aliqua nisi aliqua ullamco velit sint non proident aute quis reprehenderit ea. Aute velit consequat qui est. Mollit ipsum eu cupidatat esse laboris eiusmod officia quis quis nisi fugiat sint ut. Sint voluptate nisi sunt proident elit irure.\r\n","registered":"2019-01-26T06:17:58 -03:00","latitude":72.746999,"longitude":-129.449557,"tags":["Lorem","non","dolor","velit","tempor","do","eu"],"friends":[{"id":0,"name":"Nunez Luna"},{"id":1,"name":"Roman Aguilar"},{"id":2,"name":"Fischer Phelps"}],"greeting":"Hello, Prince Mcdowell! You have 4 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e913235fa8186b9cb56","index":2,"guid":"645c6c52-d527-49d9-b32e-6d23ed8448ce","isActive":false,"balance":"$1,192.59","picture":"http://placehold.it/32x32","age":35,"eyeColor":"brown","name":"Sandoval Barr","gender":"male","company":"QUORDATE","email":"sandovalbarr@quordate.com","phone":"+1 (883) 499-2823","address":"120 Village Road, Ribera, Vermont, 8987","about":"Proident aliquip nisi magna ipsum velit ex ut qui adipisicing ullamco. Sint aute consectetur aute occaecat minim aliquip in aliquip. Lorem sunt elit dolore ad laboris laboris aliquip anim id duis veniam. Reprehenderit labore ad nulla esse elit reprehenderit officia dolore Lorem. Voluptate occaecat est quis adipisicing dolor deserunt nostrud sunt magna sit.\r\n","registered":"2018-05-23T01:03:13 -03:00","latitude":9.736122,"longitude":-95.864018,"tags":["veniam","do","do","irure","aliqua","mollit","dolor"],"friends":[{"id":0,"name":"Kate Chavez"},{"id":1,"name":"Ratliff Huff"},{"id":2,"name":"Francine Buchanan"}],"greeting":"Hello, Sandoval Barr! You have 1 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e91104554540c61dcb2","index":3,"guid":"37daf65f-0992-49e0-9761-4ab2d233e78f","isActive":true,"balance":"$1,827.81","picture":"http://placehold.it/32x32","age":31,"eyeColor":"blue","name":"Holman Burnett","gender":"male","company":"CORECOM","email":"holmanburnett@corecom.com","phone":"+1 (882) 568-2442","address":"538 Everit Street, Strykersville, North Carolina, 5848","about":"Est incididunt velit quis dolor incididunt excepteur sit eu ex id ipsum consequat elit minim. Tempor id dolore voluptate consequat voluptate et commodo culpa dolor. Minim amet officia enim laborum exercitation aliqua qui enim dolor magna sint cupidatat quis. Magna incididunt consequat laborum dolor incididunt fugiat ea. Culpa occaecat anim ea sint consequat Lorem labore sint. Exercitation Lorem sunt aliqua aliqua ut.\r\n","registered":"2019-03-16T11:06:38 -03:00","latitude":-89.051396,"longitude":-60.91399,"tags":["culpa","ut","mollit","adipisicing","nostrud","aliqua","nulla"],"friends":[{"id":0,"name":"Rowe Goodman"},{"id":1,"name":"Romero Sharpe"},{"id":2,"name":"Nina Justice"}],"greeting":"Hello, Holman Burnett! You have 1 unread messages.","favoriteFruit":"strawberry"},
			  {"_id":"5f995e91d03c1544744b25ec","index":4,"guid":"3bb0e185-3e62-4f42-982f-886b09cc777b","isActive":true,"balance":"$2,029.32","picture":"http://placehold.it/32x32","age":23,"eyeColor":"blue","name":"Acevedo Finch","gender":"male","company":"SPRINGBEE","email":"acevedofinch@springbee.com","phone":"+1 (963) 465-2586","address":"308 Brooklyn Avenue, Baker, Utah, 3673","about":"Lorem consectetur velit excepteur ad laboris non proident excepteur Lorem. Et anim deserunt nisi eiusmod mollit amet et velit fugiat. Minim ad aliquip anim aute.\r\n","registered":"2018-05-17T03:22:44 -03:00","latitude":-17.841303,"longitude":43.925038,"tags":["consequat","cillum","consequat","ipsum","pariatur","aliqua","sunt"],"friends":[{"id":0,"name":"Lauren Clarke"},{"id":1,"name":"Hewitt Bean"},{"id":2,"name":"Kirby Morse"}],"greeting":"Hello, Acevedo Finch! You have 6 unread messages.","favoriteFruit":"banana"}
			]
 		`)

		written, err := c.Write(payload)
		require.NoError(t, err)
		assert.Equal(t, len(payload), written)

		c.Header().Set("Content-Type", "text/plain")

		err = c.Close()
		require.NoError(t, err)

		assert.Equal(t, "text/plain", w.Header().Get("Content-Type"))
		assert.Equal(t, "gzip", w.Header().Get("Content-Encoding"))
		assert.Equal(t, "2123", w.Header().Get("Content-Length"))
		assert.NotEqual(t, payload, w.Body.Bytes())

		// decode body
		cr, err := gzip.NewReader(w.Body)
		require.NoError(t, err)

		var r bytes.Buffer
		_, err = r.ReadFrom(cr)
		require.NoError(t, err)
		assert.Equal(t, payload, r.Bytes())
	})
}

func TestNewGzipCompressor(t *testing.T) {
	testCases := []struct {
		name  string
		level int
		want  *GzipCompressor
	}{
		{
			name:  "min_out_of_bounds_level",
			level: gzip.DefaultCompression - 1,
			want: func() *GzipCompressor {
				buf := new(bytes.Buffer)
				gw, err := gzip.NewWriterLevel(buf, gzip.DefaultCompression)
				require.NoError(t, err)
				return &GzipCompressor{
					gw:  gw,
					buf: buf,
				}
			}(),
		},
		{
			name:  "max_out_of_bounds_level",
			level: gzip.BestCompression + 1,
			want: func() *GzipCompressor {
				buf := new(bytes.Buffer)
				gw, err := gzip.NewWriterLevel(buf, gzip.BestCompression)
				require.NoError(t, err)
				return &GzipCompressor{
					gw:  gw,
					buf: buf,
				}
			}(),
		},
		{
			name:  "allowed_level",
			level: 5,
			want: func() *GzipCompressor {
				buf := new(bytes.Buffer)
				gw, err := gzip.NewWriterLevel(buf, 5)
				require.NoError(t, err)
				return &GzipCompressor{
					gw:  gw,
					buf: buf,
				}
			}(),
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			got := NewGzipCompressor(tc.level)

			opts := cmp.Options{
				cmp.AllowUnexported(GzipCompressor{}, gzip.Writer{}),
				cmpopts.IgnoreUnexported(bytes.Buffer{}),
			}
			assert.True(t, cmp.Equal(tc.want, got, opts...), cmp.Diff(tc.want, got, opts...))
		})
	}
}

func TestGzipCompressor_Setup(t *testing.T) {
	w := httptest.NewRecorder()
	c := NewGzipCompressor(5)
	c.Setup(w)

	expected := func() *GzipCompressor {
		buf := new(bytes.Buffer)
		gw, err := gzip.NewWriterLevel(buf, 5)
		require.NoError(t, err)
		return &GzipCompressor{
			rw:  w,
			gw:  gw,
			buf: buf,
		}
	}()

	opts := cmp.Options{
		cmp.AllowUnexported(GzipCompressor{}, gzip.Writer{}),
		cmpopts.IgnoreUnexported(bytes.Buffer{}, httptest.ResponseRecorder{}),
	}
	assert.True(t, cmp.Equal(expected, c, opts...), cmp.Diff(expected, c, opts...))

	// ensure original http writer set properly
	assert.Same(t, c.rw, w)
}

func TestGzipCompressor_Header(t *testing.T) {
	w := httptest.NewRecorder()
	c := NewGzipCompressor(5)
	c.Setup(w)

	c.Header().Set("Content-Type", "ololo")

	// ensure original http writer set properly
	assert.Equal(t, c.Header(), w.Header())
	assert.Equal(t, fmt.Sprintf("%p", c.Header()), fmt.Sprintf("%p", w.Header()))
}

func TestGzipCompressor_WriteHeader(t *testing.T) {
	w := httptest.NewRecorder()
	c := NewGzipCompressor(5)
	c.Setup(w)

	c.WriteHeader(http.StatusInternalServerError)
	assert.Equal(t, http.StatusInternalServerError, w.Code)
}

func TestGzipCompressor_Close(t *testing.T) {
	w := httptest.NewRecorder()
	c := NewGzipCompressor(5)
	c.Setup(w)

	_, err := c.Write(testBufferGen(gzipMinSize))
	require.NoError(t, err)

	err = c.Close()
	require.NoError(t, err)

	assert.Equal(t, 0, c.buf.Len())
	assert.Equal(t, 0, c.size)
}
