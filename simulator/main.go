package main

import (
	"net"
	"time"
)

func advertise(what string){
	con, err := net.Dial("udp", "255.255.255.255:40000")
	if err != nil {
		panic(err)
	}
	for{
		con.Write([]byte(what))
		time.Sleep(5 * time.Second)
	};
};
func main(){
	advertise("oscilloscope here")

}
