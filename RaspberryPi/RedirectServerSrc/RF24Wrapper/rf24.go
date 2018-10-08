package RF24Wrapper

//#cgo LDFLAGS: -L/usr/local/lib/ -lrf24
//#cgo CFLAGS: -I/usr/local/include
//#include "rf24.h"
import "C"

func Test() {
     C.Test();
}