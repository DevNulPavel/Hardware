package main

import (
    "log"
    //"io"
    "fmt"
    "os"
    "io/ioutil"
    "net/http"
    "path/filepath"
    "encoding/json"
)

const FAN_STATUS_FILE = "fan_control_out.json"
const HUMIDITY_STATUS_FILE = "humidity_control_out.json"

const MAIN_PAGE = `
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Odroid status page</title>
    <script>
        function timedRefresh(timeoutPeriod) {
            setTimeout("location.reload(true);",timeoutPeriod);
        }
    </script>
</head>
    <body onload="JavaScript:timedRefresh(1000);">
        %s
    </body>
</html>
`

type FanOut struct {
    Temperature float64 `json:"temperature"`
    Value float64 `json:"value"`
}

type HumidityOut struct {
    Humidity float64 `json:"humidity"`
    Temperature float64 `json:"temperature"`
    Alert bool `json:"alert"`
}


func getExecutableDir() string {
    dir, err := filepath.Abs(filepath.Dir(os.Args[0]))
    if err != nil {
        log.Fatal(err)
    }
    //fmt.Println(dir)
    return dir
}

func getFanOut() FanOut {
    var fanOut FanOut = FanOut{0.0, 0.0}

    jsonFilePath := getExecutableDir() + "/" + FAN_STATUS_FILE
    jsonFile, err := os.Open(jsonFilePath)
    if err != nil {
        log.Println(err)
        return fanOut
    }

    //log.Printf("%v\n", jsonFile)

    jsonBytes, _ := ioutil.ReadAll(jsonFile)
    jsonFile.Close()

    //log.Printf("%s\n", jsonBytes)

    json.Unmarshal(jsonBytes, &fanOut)
    //log.Printf("%v Out temp: %f, value: %f\n", fanOut, fanOut.Temperature, fanOut.Value)

    return fanOut
}

func getHumidityOut() HumidityOut {
    var humidityOut HumidityOut = HumidityOut{0.0, 0.0, false}

    jsonFilePath := getExecutableDir() + "/" + HUMIDITY_STATUS_FILE
    jsonFile, err := os.Open(jsonFilePath)
    if err != nil {
        log.Println(err)
        return humidityOut
    }

    //log.Printf("%v\n", jsonFile)

    jsonBytes, _ := ioutil.ReadAll(jsonFile)
    jsonFile.Close()

    //log.Printf("%s\n", jsonBytes)

    json.Unmarshal(jsonBytes, &humidityOut)
    //log.Printf("%v Out temp: %f, value: %f\n", fanOut, fanOut.Temperature, fanOut.Value)

    return humidityOut
}

func root(w http.ResponseWriter, r *http.Request) {
    fanOut := getFanOut()
    humidityOut := getHumidityOut()

    //w.Header().Set("Content-Type", response.Header.Get("Content-Type"))
    //w.Header().Set("Content-Length", response.Header.Get("Content-Length"))
    //io.Copy(w, response.Body)
    var outText string
    outText += fmt.Sprintf("CPU temp: %.1f, fan value: %.1f", fanOut.Temperature, fanOut.Value)
    outText += "<br>"
    if humidityOut.Alert == true {
        outText += `<font color="red">`
    }
    outText += fmt.Sprintf("Humidity: %.1f, temp: %.1f, alert: %t", humidityOut.Humidity, humidityOut.Temperature, humidityOut.Alert)
    if humidityOut.Alert == true {
        outText += `</font>`
    }
    outTextHTML := fmt.Sprintf(MAIN_PAGE, outText)
    fmt.Fprintf(w, outTextHTML)
}

func main() {
    http.HandleFunc("/", root)
    err := http.ListenAndServe(":8080", nil)
    if err != nil {
        log.Fatal("ListenAndServe: ", err)
    }
}
