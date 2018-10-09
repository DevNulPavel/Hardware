package RF24Wrapper

/*
#cgo LDFLAGS: -L/usr/local/lib/ -lrf24
#cgo CFLAGS: -I/usr/local/include
#include "rf24.h"
*/
import "C"

import "unsafe"
import "log"

type RF24Command int32
const (
    CMD_GET_STATUS RF24Command = RF24Command(C.CMD_GET_STATUS)
    CMD_LIGHT_AUTO RF24Command = RF24Command(C.CMD_LIGHT_AUTO)
    CMD_LIGHT_ON RF24Command = RF24Command(C.CMD_LIGHT_ON)
    CMD_LIGHT_OFF RF24Command = RF24Command(C.CMD_LIGHT_OFF)
)

type RF24Status int32
const (
    ST_LIGHT_AUTO RF24Status = RF24Status(C.ST_LIGHT_AUTO)
    ST_LIGHT_ON RF24Status = RF24Status(C.ST_LIGHT_ON)
    ST_LIGHT_OFF RF24Status = RF24Status(C.ST_LIGHT_OFF)
)

type RF24Error int32
const (
    ERR_OK RF24Error = RF24Error(C.ERR_OK)
    ERR_WRITE_FAILED RF24Error = RF24Error(C.ERR_WRITE_FAILED)
    ERR_TIMEOUT RF24Error = RF24Error(C.ERR_TIMEOUT)
    ERR_NO_RESULT_PTR RF24Error = RF24Error(C.ERR_NO_RESULT_PTR)
)

type RF24Result struct{
    status RF24Status
    lightVal uint32
}


func NewRF24Result() RF24Result {
    return RF24Result{ST_LIGHT_AUTO, 0}
}

func Test() {
    status := NewRF24Result()

    C.RF24Begin()

    commandC := (C.RF24Command)(CMD_GET_STATUS)
    resultPtrC := (*C.RF24Result)(unsafe.Pointer(&status))
    errorVal := RF24Error(C.RF24SendCommand(commandC, resultPtrC))

    ok := false
    if errorVal == ERR_OK {
        ok = true
    }

    log.Printf("RF24 ok val: %v", ok)
}