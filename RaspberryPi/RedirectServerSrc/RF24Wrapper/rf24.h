// https://stackoverflow.com/questions/1713214/how-to-use-c-in-go
// https://golang.org/cmd/cgo/

#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    CMD_GET_STATUS = 0,
    CMD_LIGHT_AUTO = 1,
    CMD_LIGHT_ON = 2,
    CMD_LIGHT_OFF = 3
} RF24Command;

typedef enum {
    ST_LIGHT_AUTO = 0,
    ST_LIGHT_ON = 1,
    ST_LIGHT_OFF = 2
} RF24Status;

typedef enum {
    ERR_OK = 0,
    ERR_WRITE_FAILED = 1,
    ERR_TIMEOUT = 2,
    ERR_NO_RESULT_PTR = 3,
} RF24Error;

typedef struct RF24Result{
    unsigned int status; // RF24Status
    unsigned int lightVal;
} RF24Result;


void RF24Begin();
void RF24Ping();
RF24Error RF24SendCommand(RF24Command command, RF24Result* result);
void TestCGo();

#ifdef __cplusplus
}
#endif