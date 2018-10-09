package RF24Wrapper

/*
#cgo LDFLAGS: -L/usr/local/lib/ -lrf24
#cgo CFLAGS: -I/usr/local/include
#include "rf24.h"
*/
import "C"

import "unsafe"
import "fmt"

type RF24Command uint8
const (
    CMD_GET_STATUS RF24Command = RF24Command(C.CMD_GET_STATUS)
    CMD_LIGHT_AUTO RF24Command = RF24Command(C.CMD_LIGHT_AUTO)
    CMD_LIGHT_ON RF24Command = RF24Command(C.CMD_LIGHT_ON)
    CMD_LIGHT_OFF RF24Command = RF24Command(C.CMD_LIGHT_OFF)
)

type RF24Status uint8
const (
    ST_LIGHT_AUTO RF24Status = RF24Status(C.ST_LIGHT_AUTO)
    ST_LIGHT_ON RF24Status = RF24Status(C.ST_LIGHT_ON)
    ST_LIGHT_OFF RF24Status = RF24Status(C.ST_LIGHT_OFF)
)

type RF24Error uint8
const (
    ERR_OK RF24Error = RF24Error(C.ERR_OK)
    ERR_WRITE_FAILED RF24Error = RF24Error(C.ERR_WRITE_FAILED)
    ERR_TIMEOUT RF24Error = RF24Error(C.ERR_TIMEOUT)
    ERR_NO_RESULT_PTR RF24Error = RF24Error(C.ERR_NO_RESULT_PTR)
)

type RF24Result struct{
    Status RF24Status
    Enabled uint8
    LightVal uint16
}


func NewRF24Result() RF24Result {
    return RF24Result{ST_LIGHT_AUTO, 0, 0}
}

func BeginRF24(){
    C.RF24Begin()    
}

func SendCommandToRF24(command RF24Command) (RF24Result, error) {
    status := NewRF24Result()

    commandC := (C.RF24Command)(command)
    resultPtrC := (*C.RF24Result)(unsafe.Pointer(&status))
    errorVal := RF24Error(C.RF24SendCommand(commandC, resultPtrC))

    if errorVal != ERR_OK {
        return NewRF24Result(), fmt.Errorf("RF24 command error: %d", errorVal)
    }

    return status, nil
}
