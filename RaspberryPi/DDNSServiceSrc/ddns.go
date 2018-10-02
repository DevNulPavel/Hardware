package main

import (
    "fmt"
    "os"
    "log"
    "path"
    "path/filepath"
    "io/ioutil"
    "net/http"
)

// devnulpavel.hldns.ru
const ADDRESS = "http://hldns.ru/update/6RA09KTFQ6OYQWK7EKT63YOCXL0NSY"

func main() {
    // Exec folder
    dir, err := filepath.Abs(filepath.Dir(os.Args[0]))
    if err != nil {
        log.Fatal(err)
    }

    // Log output
    logFile, err := os.OpenFile(path.Join(dir, "ddns.log"), os.O_RDWR | os.O_CREATE | os.O_APPEND, 0666)
    if err != nil {
        log.Fatalf("Error opening file: %v", err)
    }
    defer logFile.Close()
    log.SetOutput(logFile)

    // Result output
    outFile, err := os.OpenFile(path.Join(dir, "ddns_last_out.log"), os.O_RDWR | os.O_CREATE, 0666)
    if err != nil {
        log.Fatalf("Error opening file: %v", err)
    }
    defer outFile.Close()
    outFile.Truncate(0)
    outFile.Seek(0,0)

    // Request
    response, err := http.Get(ADDRESS)
    if err != nil {
        log.Printf("%s", err)
    } else if response != nil {
        defer response.Body.Close()
        if response.StatusCode == http.StatusOK {
            bodyBytes, _ := ioutil.ReadAll(response.Body)
            bodyString := string(bodyBytes)
            bodyString += "\n"
            fmt.Printf("%s", bodyString)
            outFile.WriteString(bodyString)
        }else{
            log.Printf("Invalid responce status: %d", response.StatusCode)
        }
    }else{
        log.Printf("No server responce")
    }
}