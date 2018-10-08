package rf24


//#include<stdio.h>
//void Test() {
//    printf("I am in C code now!\n");
//}
import "C"

func Test() {
     C.Test();
}

//#cgo LDFLAGS: -L/usr/lib/RF24
//#cgo LDFLAGS: -lrf24
//#cgo CFLAGS: -I/usr/include/RF24
//#include "rf24.h"