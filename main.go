package main

import (
	"encoding/json"
	"fmt"
	"log"
	"maps"
	"net/http"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/go-chi/chi/v5"
	"github.com/go-chi/chi/v5/middleware"
)

var messageHandler mqtt.MessageHandler = func(c mqtt.Client, m mqtt.Message) {
	message := m.Payload()
	data := make(map[string]interface{})
	err := json.Unmarshal(message, &data)
	// fmt.Println(string(m.Payload()))
	if err != nil {
		fmt.Println(err)
	}

	for keys := range maps.Values(data) {
		fmt.Println(keys)
	}

}

var connectionHandler mqtt.OnConnectHandler = func(c mqtt.Client) {
	fmt.Println("connected")
}

func main() {

	// print("Hello World")
	ops := mqtt.NewClientOptions()
	ops.AddBroker("tcp://broker.emqx.io:1883")
	ops.OnConnect = connectionHandler
	ops.SetDefaultPublishHandler(messageHandler)

	client := mqtt.NewClient(ops)
	if token := client.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	token := client.Subscribe("hello/world", 1, nil)
	token.Wait()

	// client.

	r := chi.NewRouter()

	r.Use(middleware.Logger)
	r.Get("/", func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello World"))
	})

	err := http.ListenAndServe(":3000", r)
	if err != nil {
		log.Println(err)
	}

	client.Disconnect(250)

}
