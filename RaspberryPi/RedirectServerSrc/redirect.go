package main

import (
    "log"
    "io"
    "bytes"
    "fmt"
    "strconv"
    "strings"
    "os/exec"
    "encoding/gob"
    "net/http"
    "image/jpeg"
    //"github.com/nfnt/resize"
    "./resize"
    //"github.com/gorilla/sessions"
    "./sessions"
)

const ODROID_ADDRESS = "http://192.168.1.7:8080"
const ESP_ADDRESS = "http://192.168.1.9:80"
const DIGMA_ADDRESS = "http://192.168.1.3:8888"
const DIGMA_LOGIN = "DevNul1"
const DIGMA_PASS = "Azsxdcfv1234"
const DIGMA_LOGIN_AND_DIGMA_PASS = "loginuse="+DIGMA_LOGIN+"&loginpas="+DIGMA_PASS
const SERVER_LOGIN = "DevNul"
const SERVER_PASS = "azsxdcfv"
const SERVER_LOGIN_PASS = SERVER_LOGIN+":"+SERVER_PASS


//////////////////////////////////////////
// Main page

const MAIN_PAGE = `
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Main page</title>
</head>
<body>
    <p><a href="esp">ESP8268 status</a></p>
    <p><a href="digma_image">Digma image</a></p>
    <p><a href="digma_small">Digma small image</a></p>
    <p><a href="digma_video">Digma video</a></p>
    <p><a href="pi_image?shutter_time_ms=4000&iso=800">Pi image (Night)</a></p>
    <p><a href="pi_image?shutter_time_ms=1000&iso=160">Pi image (Day)</a></p>
    <p><a href="pi_custom">Pi image (Custom)</a></p>
</body>
</html>
`

//////////////////////////////////////////
// Login page

const LOGIN_PAGE = `
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Main page</title>
</head>
<body>
    <div>
      <div>
        <form action="/login">
          <input type="text" placeholder="Username" name="login">
          <input type="text" placeholder="Password" name="pass">
          <button type="submit">Login</button>
        </form>
      </div>
    </div>
</body>
</html>
`

//////////////////////////////////////////
// Login page

const CUSTOM_PI_CAMERA_PAGE = `
<html>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title>Main page</title>
</head>
<body>
    <div>
      <div>
        <form action="/pi_image" method="get">
          Shutter time (mSec): <input type="text" placeholder="in mSec" value="330" name="shutter_time_ms">
          ISO: <input type="text" placeholder="100-800" value="800" name="iso">
          <br>
          <button type="submit">Go to image</button>
        </form>
      </div>
    </div>
</body>
</html>
`

//////////////////////////////////////////
// Login support

type sesKey int
var cookieStore = sessions.NewCookieStore([]byte("secret"))
const (
    cookieName = "PassCookie"
    sesKeyLoginPas sesKey = iota
)

//////////////////////////////////////////
// Code

func checkLogin(w http.ResponseWriter, r *http.Request) bool {
    ses, err := cookieStore.Get(r, cookieName)
    if err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return false
    }

    loginAndPass, ok := ses.Values[sesKeyLoginPas].(string)
    if !ok {
        // Redirect to login page
        http.Redirect(w, r, "/login", http.StatusFound)
        return false
    }
    if ok && loginAndPass != SERVER_LOGIN_PASS {
        // Redirect to login page
        http.Redirect(w, r, "/login", http.StatusFound)
        return false
    }

    return true
}


func makeDigmaSetupRequest() (err error){
    _, err = http.Get(DIGMA_ADDRESS+"/camera_control.cgi?"+DIGMA_LOGIN_AND_DIGMA_PASS+"&param=9&value=0")
    if err != nil {
        return err
    }
    _, err = http.Get(DIGMA_ADDRESS+"/camera_control.cgi?"+DIGMA_LOGIN_AND_DIGMA_PASS+"&param=6&value=25")
    if err != nil {
        return err
    }
    return nil
}

func sharedDigmaHandler(w http.ResponseWriter, r *http.Request, command string) {
    if !checkLogin(w, r) {
        return
    }
    
    err := makeDigmaSetupRequest()
    if err != nil {
        log.Printf("%s", err)
        return
    }

    //response, err := http.Get("http://192.168.1.3:8888/?DIGMA_LOGINuse=DevNul1&DIGMA_LOGINpas=Azsxdcfv1234")
    response, err := http.Get(DIGMA_ADDRESS+"/"+command+"?"+DIGMA_LOGIN_AND_DIGMA_PASS)
    if err != nil {
        log.Printf("%s", err)
        return
    } else {
        defer response.Body.Close()
        w.Header().Set("Content-Type", response.Header.Get("Content-Type"))
        w.Header().Set("Content-Length", response.Header.Get("Content-Length"))
        io.Copy(w, response.Body)
    }
}

func digmaImage(w http.ResponseWriter, r *http.Request) {
    sharedDigmaHandler(w, r, "snapshot.cgi")
}

func digmaVideo(w http.ResponseWriter, r *http.Request) {
    sharedDigmaHandler(w, r, "videostream.cgi")
}

func digmaImageSmall(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }
    //log.Printf("New request")
    
    err := makeDigmaSetupRequest()
    if err != nil {
        log.Printf("%s", err)
        return
    }

    //response, err := http.Get("http://192.168.1.3:8888/?DIGMA_LOGINuse=DevNul1&DIGMA_LOGINpas=Azsxdcfv1234")
    response, err := http.Get(DIGMA_ADDRESS+"/snapshot.cgi?"+DIGMA_LOGIN_AND_DIGMA_PASS)
    if err == nil {
        defer response.Body.Close()

        imgJpg, _ := jpeg.Decode(response.Body)

        imgJpg = resize.Resize(640, 0, imgJpg, resize.Bicubic)

        buf := new(bytes.Buffer) // TODO: Отжирает память, может быть можно избавиться? но как получить размер?
        jpeg.Encode(buf, imgJpg, nil)

        w.Header().Set("Content-Type", response.Header.Get("Content-Type"))
        w.Header().Set("Content-Length", strconv.Itoa(buf.Len()))

        io.Copy(w, buf)

    } else {
        log.Printf("%s", err)
        return
    }
}

func odroid(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }

    path := r.URL.Path
    path = strings.Replace(path, "/odroid", "", 1)
    response, err := http.Get(ODROID_ADDRESS+path)
    if err == nil {
        defer response.Body.Close()
        w.Header().Set("Content-Type", response.Header.Get("Content-Type"))
        w.Header().Set("Content-Length", response.Header.Get("Content-Length"))
        io.Copy(w, response.Body)
    } else {
        log.Printf("%s", err)
        return
    }
}

func esp(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }

    //path := r.URL.Path
    //path = strings.Replace(path, "/esp", "", 1)
    //response, err := http.Get(ESP_ADDRESS+path)
    response, err := http.Get(ESP_ADDRESS)
    if err == nil {
        defer response.Body.Close()

        buf := new(bytes.Buffer)
        io.Copy(buf, response.Body)

        w.Header().Set("Content-Type", response.Header.Get("Content-Type"))
        w.Header().Set("Content-Length", strconv.Itoa(buf.Len()))
        io.Copy(w, buf)
    } else {
        log.Printf("%s", err)
        return
    }
}

func getPiCameraImage(shutterTimeMSec uint32, iso uint32) ([]byte, error) {
    commandText := fmt.Sprintf("raspistill -vf -hf -ss %d -ISO %d -o -", shutterTimeMSec*1000, iso)
    command := exec.Command("bash", "-c", commandText)
    imageResult, err := command.Output()
    if err != nil {
        log.Printf("%s", err)
        return nil, err
    }

    return imageResult, nil
}

func piImage(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }

    var shutterTime uint32 = 330
    var iso uint32 = 800
    
    shutterTimeGet, ok := r.URL.Query()["shutter_time_ms"]
    if ok && len(shutterTimeGet[0]) > 0 {
        parsedVal, err := strconv.ParseInt(shutterTimeGet[0], 10, 32)
        if err == nil {
            shutterTime = uint32(parsedVal)
        }
    }

    isoGet, ok := r.URL.Query()["iso"]
    if ok && len(isoGet[0]) > 0 {
        parsedVal, err := strconv.ParseInt(isoGet[0], 10, 32)
        if err == nil {
            iso = uint32(parsedVal)
        }
    }

    //log.Printf("Pi image request params: %d %d", shutterTime, iso)

    imageData, imageError := getPiCameraImage(shutterTime, iso)
    if imageError == nil {
        w.Header().Set("Content-Type", "image/jpeg")
        w.Header().Set("Content-Length", strconv.Itoa(len(imageData)))
        w.Write(imageData)
    }else{
        errorString := imageError.Error()
        w.Header().Set("Content-Type", "text/html")
        w.Header().Set("Content-Length", strconv.Itoa(len(errorString)))
        io.WriteString(w, errorString)
    }
}

func piImageCustom(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }
    fmt.Fprint(w, CUSTOM_PI_CAMERA_PAGE)
}

func login(w http.ResponseWriter, r *http.Request) {
    ses, err := cookieStore.Get(r, cookieName)
    if err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return
    }

    loginAndPass, ok := ses.Values[sesKeyLoginPas].(string)
    if ok && (loginAndPass == SERVER_LOGIN_PASS) {
        //fmt.Fprint(w, "Already have login")
        // Redirect to main page
        http.Redirect(w, r, "/", http.StatusFound)
        return
    }

    login, ok := r.URL.Query()["login"]
    if !ok || (len(login[0]) == 0) {
        fmt.Fprint(w, LOGIN_PAGE)
        return
    }
    pass, ok := r.URL.Query()["pass"]
    if !ok || (len(pass[0]) == 0) {
        fmt.Fprint(w, LOGIN_PAGE)
        return
    }

    loginAndPass = login[0]+":"+pass[0]

    if (loginAndPass == SERVER_LOGIN_PASS) {
        ses.Values[sesKeyLoginPas] = SERVER_LOGIN_PASS
        err = cookieStore.Save(r, w, ses)
        if err != nil {
            http.Error(w, err.Error(), http.StatusBadRequest)
            return
        }
        // Redirect to main page
        http.Redirect(w, r, "/", http.StatusFound)
    }
}

// Простая страничка со ссылками
func root(w http.ResponseWriter, r *http.Request) {
    if !checkLogin(w, r) {
        return
    }
    fmt.Fprint(w, MAIN_PAGE)
}

func main() {
    gob.Register(sesKey(0))

    http.HandleFunc("/", root)
    http.HandleFunc("/login", login)
    http.HandleFunc("/odroid", odroid)
    http.HandleFunc("/esp", esp)
    http.HandleFunc("/digma_image", digmaImage)
    http.HandleFunc("/digma_video", digmaVideo)
    http.HandleFunc("/digma_small", digmaImageSmall)
    http.HandleFunc("/pi_image", piImage)
    http.HandleFunc("/pi_custom", piImageCustom)
    err := http.ListenAndServe(":8080", nil)
    if err != nil {
        log.Fatal("ListenAndServe: ", err)
    }
}