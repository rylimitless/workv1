package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
	"github.com/gorilla/websocket"
)

type Sockets struct {
	mu          sync.Mutex
	connections map[*websocket.Conn]bool
}

var (
	sockets        Sockets
	messageHandler mqtt.MessageHandler = func(c mqtt.Client, m mqtt.Message) {
		message := m.Payload()
		data := make(map[string]interface{})
		err := json.Unmarshal(message, &data)
		// fmt.Println(string(m.Payload()))
		if err != nil {
			fmt.Println(err)
		}

		sockets.mu.Lock()
		defer sockets.mu.Unlock()

		for connection, _ := range sockets.connections {
			connection.WriteJSON(data)
		}

	}

	upgrader = websocket.Upgrader{
		CheckOrigin: func(r *http.Request) bool {
			// Allow all origins during development
			return true
		},
	}
	connectionHandler mqtt.OnConnectHandler = func(c mqtt.Client) {
		fmt.Println("connected")
	}
)

func (s *Sockets) Subscribe(client mqtt.Client) {
	token := client.Subscribe("hello/world", 1, nil)
	token.Wait()
}

func main() {

	sockets = Sockets{
		connections: make(map[*websocket.Conn]bool),
	}

	// print("Hello World")
	ops := mqtt.NewClientOptions()
	ops.AddBroker("tcp://broker.emqx.io:1883")
	ops.OnConnect = connectionHandler
	ops.SetDefaultPublishHandler(messageHandler)

	client := mqtt.NewClient(ops)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	sockets.Subscribe(client)

	// client.

	r := chi.NewRouter()

	r.Use(middleware.Logger)
	r.Get("/sock", func(w http.ResponseWriter, r *http.Request) {
		c, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Print("upgrade:", err)
			return
		}
		defer c.Close()

		sockets.connections[c] = true

		for {
		}
	})

	err := http.ListenAndServe(":3000", r)
	if err != nil {
		log.Println(err)
	}

	client.Disconnect(250)

}
