package compress

import (
	"bytes"
	"compress/gzip"
	"net/http"
	"net/http/httptest"
	"strconv"
	"testing"

	"github.com/go-chi/chi/v5/middleware"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestCompressHandler_wrap(t *testing.T) {
	testCases := []struct {
		name        string
		handler     func(http.Handler) http.Handler
		next        http.Handler
		request     *http.Request
		wantStatus  int
		wantHeaders http.Header
		uncompress  func(body []byte) error
	}{
		{
			name:    "no_accept_encoding",
			handler: NewHandler(5),
			next: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				w.WriteHeader(http.StatusCreated)
			}),
			request: func() *http.Request {
				req, _ := http.NewRequest("GET", "/", nil)
				return req
			}(),
			wantStatus:  http.StatusCreated,
			wantHeaders: http.Header{},
			uncompress: func(body []byte) error {
				return nil
			},
		},
		{
			name:    "unsupported_accept_encoding",
			handler: NewHandler(5),
			next: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				w.WriteHeader(http.StatusCreated)
			}),
			request: func() *http.Request {
				req, _ := http.NewRequest("GET", "/", nil)
				req.Header.Set("Accept-Encoding", "my-super-compressor")
				return req
			}(),
			wantStatus:  http.StatusCreated,
			wantHeaders: http.Header{},
			uncompress: func(body []byte) error {
				return nil
			},
		},
		{
			name:    "gzip_uncompressed",
			handler: NewHandler(5),
			next: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, err := w.Write(testBufferGen(gzipMinSize - 1))
				require.NoError(t, err)
				w.WriteHeader(http.StatusCreated)
			}),
			request: func() *http.Request {
				req, _ := http.NewRequest("GET", "/", nil)
				req.Header.Set("Accept-Encoding", "gzip")
				return req
			}(),
			wantStatus: http.StatusCreated,
			wantHeaders: http.Header{
				"Content-Length": []string{strconv.Itoa(gzipMinSize - 1)},
			},
			uncompress: func(body []byte) error {
				return nil
			},
		},
		{
			name:    "gzip_compressed",
			handler: NewHandler(5),
			next: http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, err := w.Write(testBufferFill([]byte{'0'}, 1024*1024))
				require.NoError(t, err)
				w.WriteHeader(http.StatusCreated)
			}),
			request: func() *http.Request {
				req, _ := http.NewRequest("GET", "/", nil)
				req.Header.Set("Accept-Encoding", "gzip")
				return req
			}(),
			wantStatus: http.StatusCreated,
			wantHeaders: http.Header{
				"Content-Encoding": []string{"gzip"},
				"Content-Length":   []string{"1055"},
			},
			uncompress: func(body []byte) error {
				cr, err := gzip.NewReader(bytes.NewBuffer(body))
				if err != nil {
					return err
				}
				var r bytes.Buffer
				_, err = r.ReadFrom(cr)
				return err
			},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			w := httptest.NewRecorder()

			wrapped := tc.handler(tc.next)
			wrapped.ServeHTTP(w, tc.request)

			assert.Equal(t, tc.wantStatus, w.Code)
			assert.Equal(t, tc.wantHeaders, w.Header())
			assert.NoError(t, tc.uncompress(w.Body.Bytes()))
		})
	}
}

func BenchmarkCompress(b *testing.B) {
	bufs := [][]byte{
		[]byte(`{"_id":"5f995e91af87f5b6d333d41f","index":0}`),
		[]byte(`{"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868"}`),
		[]byte(`{"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO"}`),
		[]byte(`{"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO","email":"juanitataylor@vortexaco.com","phone":"+1 (975) 500-3334","address":"371 Middleton Street, Walland, New Hampshire, 326","about":"Exercitation dolore excepteur quis ad Lorem irure velit nisi nostrud pariatur. Dolor elit esse voluptate sunt est culpa. Ea dolor velit irure do laborum consequat. Excepteur aliqua anim elit proident ad enim ea ea dolor eiusmod voluptate.\r\n","registered":"2014-03-14T03:48:21 -04:00","latitude":-18.303385,"longitude":169.562065,"tags":["culpa","est","do","ea","ullamco","pariatur","do"],"friends":[{"id":0,"name":"Lily Ramsey"},{"id":1,"name":"Christensen Hahn"},{"id":2,"name":"Berta Murray"}],"greeting":"Hello, Juanita Taylor! You have 9 unread messages.","favoriteFruit":"banana"}`),
		[]byte(`
			[
				{"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO","email":"juanitataylor@vortexaco.com","phone":"+1 (975) 500-3334","address":"371 Middleton Street, Walland, New Hampshire, 326","about":"Exercitation dolore excepteur quis ad Lorem irure velit nisi nostrud pariatur. Dolor elit esse voluptate sunt est culpa. Ea dolor velit irure do laborum consequat. Excepteur aliqua anim elit proident ad enim ea ea dolor eiusmod voluptate.\r\n","registered":"2014-03-14T03:48:21 -04:00","latitude":-18.303385,"longitude":169.562065,"tags":["culpa","est","do","ea","ullamco","pariatur","do"],"friends":[{"id":0,"name":"Lily Ramsey"},{"id":1,"name":"Christensen Hahn"},{"id":2,"name":"Berta Murray"}],"greeting":"Hello, Juanita Taylor! You have 9 unread messages.","favoriteFruit":"banana"},
				{"_id":"5f995e91892fc5c887bf75f1","index":1,"guid":"89addd5e-12d6-4ebb-bf48-1da40437febf","isActive":false,"balance":"$3,288.49","picture":"http://placehold.it/32x32","age":24,"eyeColor":"blue","name":"Prince Mcdowell","gender":"male","company":"ZAJ","email":"princemcdowell@zaj.com","phone":"+1 (805) 582-3382","address":"976 Ingraham Street, Ironton, Alaska, 3932","about":"Eiusmod laborum id in et excepteur mollit voluptate enim enim. Aliqua nisi aliqua ullamco velit sint non proident aute quis reprehenderit ea. Aute velit consequat qui est. Mollit ipsum eu cupidatat esse laboris eiusmod officia quis quis nisi fugiat sint ut. Sint voluptate nisi sunt proident elit irure.\r\n","registered":"2019-01-26T06:17:58 -03:00","latitude":72.746999,"longitude":-129.449557,"tags":["Lorem","non","dolor","velit","tempor","do","eu"],"friends":[{"id":0,"name":"Nunez Luna"},{"id":1,"name":"Roman Aguilar"},{"id":2,"name":"Fischer Phelps"}],"greeting":"Hello, Prince Mcdowell! You have 4 unread messages.","favoriteFruit":"strawberry"},
				{"_id":"5f995e913235fa8186b9cb56","index":2,"guid":"645c6c52-d527-49d9-b32e-6d23ed8448ce","isActive":false,"balance":"$1,192.59","picture":"http://placehold.it/32x32","age":35,"eyeColor":"brown","name":"Sandoval Barr","gender":"male","company":"QUORDATE","email":"sandovalbarr@quordate.com","phone":"+1 (883) 499-2823","address":"120 Village Road, Ribera, Vermont, 8987","about":"Proident aliquip nisi magna ipsum velit ex ut qui adipisicing ullamco. Sint aute consectetur aute occaecat minim aliquip in aliquip. Lorem sunt elit dolore ad laboris laboris aliquip anim id duis veniam. Reprehenderit labore ad nulla esse elit reprehenderit officia dolore Lorem. Voluptate occaecat est quis adipisicing dolor deserunt nostrud sunt magna sit.\r\n","registered":"2018-05-23T01:03:13 -03:00","latitude":9.736122,"longitude":-95.864018,"tags":["veniam","do","do","irure","aliqua","mollit","dolor"],"friends":[{"id":0,"name":"Kate Chavez"},{"id":1,"name":"Ratliff Huff"},{"id":2,"name":"Francine Buchanan"}],"greeting":"Hello, Sandoval Barr! You have 1 unread messages.","favoriteFruit":"strawberry"}
			]
		`),
		[]byte(`
			[
				{"_id":"5f995e91af87f5b6d333d41f","index":0,"guid":"bc1eee03-cf2c-4d68-8182-f53b6962e868","isActive":true,"balance":"$3,834.60","picture":"http://placehold.it/32x32","age":33,"eyeColor":"blue","name":"Juanita Taylor","gender":"female","company":"VORTEXACO","email":"juanitataylor@vortexaco.com","phone":"+1 (975) 500-3334","address":"371 Middleton Street, Walland, New Hampshire, 326","about":"Exercitation dolore excepteur quis ad Lorem irure velit nisi nostrud pariatur. Dolor elit esse voluptate sunt est culpa. Ea dolor velit irure do laborum consequat. Excepteur aliqua anim elit proident ad enim ea ea dolor eiusmod voluptate.\r\n","registered":"2014-03-14T03:48:21 -04:00","latitude":-18.303385,"longitude":169.562065,"tags":["culpa","est","do","ea","ullamco","pariatur","do"],"friends":[{"id":0,"name":"Lily Ramsey"},{"id":1,"name":"Christensen Hahn"},{"id":2,"name":"Berta Murray"}],"greeting":"Hello, Juanita Taylor! You have 9 unread messages.","favoriteFruit":"banana"},
				{"_id":"5f995e91892fc5c887bf75f1","index":1,"guid":"89addd5e-12d6-4ebb-bf48-1da40437febf","isActive":false,"balance":"$3,288.49","picture":"http://placehold.it/32x32","age":24,"eyeColor":"blue","name":"Prince Mcdowell","gender":"male","company":"ZAJ","email":"princemcdowell@zaj.com","phone":"+1 (805) 582-3382","address":"976 Ingraham Street, Ironton, Alaska, 3932","about":"Eiusmod laborum id in et excepteur mollit voluptate enim enim. Aliqua nisi aliqua ullamco velit sint non proident aute quis reprehenderit ea. Aute velit consequat qui est. Mollit ipsum eu cupidatat esse laboris eiusmod officia quis quis nisi fugiat sint ut. Sint voluptate nisi sunt proident elit irure.\r\n","registered":"2019-01-26T06:17:58 -03:00","latitude":72.746999,"longitude":-129.449557,"tags":["Lorem","non","dolor","velit","tempor","do","eu"],"friends":[{"id":0,"name":"Nunez Luna"},{"id":1,"name":"Roman Aguilar"},{"id":2,"name":"Fischer Phelps"}],"greeting":"Hello, Prince Mcdowell! You have 4 unread messages.","favoriteFruit":"strawberry"},
				{"_id":"5f995e913235fa8186b9cb56","index":2,"guid":"645c6c52-d527-49d9-b32e-6d23ed8448ce","isActive":false,"balance":"$1,192.59","picture":"http://placehold.it/32x32","age":35,"eyeColor":"brown","name":"Sandoval Barr","gender":"male","company":"QUORDATE","email":"sandovalbarr@quordate.com","phone":"+1 (883) 499-2823","address":"120 Village Road, Ribera, Vermont, 8987","about":"Proident aliquip nisi magna ipsum velit ex ut qui adipisicing ullamco. Sint aute consectetur aute occaecat minim aliquip in aliquip. Lorem sunt elit dolore ad laboris laboris aliquip anim id duis veniam. Reprehenderit labore ad nulla esse elit reprehenderit officia dolore Lorem. Voluptate occaecat est quis adipisicing dolor deserunt nostrud sunt magna sit.\r\n","registered":"2018-05-23T01:03:13 -03:00","latitude":9.736122,"longitude":-95.864018,"tags":["veniam","do","do","irure","aliqua","mollit","dolor"],"friends":[{"id":0,"name":"Kate Chavez"},{"id":1,"name":"Ratliff Huff"},{"id":2,"name":"Francine Buchanan"}],"greeting":"Hello, Sandoval Barr! You have 1 unread messages.","favoriteFruit":"strawberry"},
				{"_id":"5f995e91104554540c61dcb2","index":3,"guid":"37daf65f-0992-49e0-9761-4ab2d233e78f","isActive":true,"balance":"$1,827.81","picture":"http://placehold.it/32x32","age":31,"eyeColor":"blue","name":"Holman Burnett","gender":"male","company":"CORECOM","email":"holmanburnett@corecom.com","phone":"+1 (882) 568-2442","address":"538 Everit Street, Strykersville, North Carolina, 5848","about":"Est incididunt velit quis dolor incididunt excepteur sit eu ex id ipsum consequat elit minim. Tempor id dolore voluptate consequat voluptate et commodo culpa dolor. Minim amet officia enim laborum exercitation aliqua qui enim dolor magna sint cupidatat quis. Magna incididunt consequat laborum dolor incididunt fugiat ea. Culpa occaecat anim ea sint consequat Lorem labore sint. Exercitation Lorem sunt aliqua aliqua ut.\r\n","registered":"2019-03-16T11:06:38 -03:00","latitude":-89.051396,"longitude":-60.91399,"tags":["culpa","ut","mollit","adipisicing","nostrud","aliqua","nulla"],"friends":[{"id":0,"name":"Rowe Goodman"},{"id":1,"name":"Romero Sharpe"},{"id":2,"name":"Nina Justice"}],"greeting":"Hello, Holman Burnett! You have 1 unread messages.","favoriteFruit":"strawberry"},
				{"_id":"5f995e91d03c1544744b25ec","index":4,"guid":"3bb0e185-3e62-4f42-982f-886b09cc777b","isActive":true,"balance":"$2,029.32","picture":"http://placehold.it/32x32","age":23,"eyeColor":"blue","name":"Acevedo Finch","gender":"male","company":"SPRINGBEE","email":"acevedofinch@springbee.com","phone":"+1 (963) 465-2586","address":"308 Brooklyn Avenue, Baker, Utah, 3673","about":"Lorem consectetur velit excepteur ad laboris non proident excepteur Lorem. Et anim deserunt nisi eiusmod mollit amet et velit fugiat. Minim ad aliquip anim aute.\r\n","registered":"2018-05-17T03:22:44 -03:00","latitude":-17.841303,"longitude":43.925038,"tags":["consequat","cillum","consequat","ipsum","pariatur","aliqua","sunt"],"friends":[{"id":0,"name":"Lauren Clarke"},{"id":1,"name":"Hewitt Bean"},{"id":2,"name":"Kirby Morse"}],"greeting":"Hello, Acevedo Finch! You have 6 unread messages.","favoriteFruit":"banana"}
			]
		`),
	}

	var handlers []http.Handler
	for _, buf := range bufs {
		buf := buf
		h := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
			_, _ = w.Write(buf)
			w.Header().Set("Content-Type", "text/plain")
			w.WriteHeader(http.StatusCreated)
		})
		handlers = append(handlers, h)
	}

	ch := NewHandler(5)
	cc := middleware.Compress(5, "text/plain")

	b.Run("compressHandler_gzip5", func(b *testing.B) {
		w := httptest.NewRecorder()
		r := httptest.NewRequest("GET", "/", nil)
		r.Header.Set("Accept-Encoding", "gzip")

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			ch(handlers[i%len(handlers)]).ServeHTTP(w, r)
		}
	})

	b.Run("chiCompress_gzip5", func(b *testing.B) {
		w := httptest.NewRecorder()
		r := httptest.NewRequest("GET", "/", nil)
		r.Header.Set("Accept-Encoding", "gzip")

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			cc(handlers[i%len(handlers)]).ServeHTTP(w, r)
		}
	})
}
