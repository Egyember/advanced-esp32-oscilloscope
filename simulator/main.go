package main

import (
	"errors"
	"fmt"
	"net"
	"time"
)

type config struct{
	channels uint8;
	sampleRate uint32; // expected ADC sampling frequency in Hz.
	duration uint32;   // in ms
};

func parseConf(buf []byte) (conf config, err error) {
	err = nil
	if len(buf) <9 {
		err =  errors.New("to short buffer")
		return
	}
	conf.channels = buf[0]
	return
}

func advertise(what string){
	con, err := net.Dial("udp", "255.255.255.255:40000")
	if err != nil {
		panic(err)
	}
	for{
		_, err = con.Write([]byte(what))
		if err != nil {
			panic(err)
		}
		time.Sleep(5 * time.Second)
	};
};
func main(){
	go advertise("oscilloscope here")
	lisen, err := net.Listen("tcp4", "0.0.0.0:40001")
	if err != nil {
		panic(err)
	}
	for {
		con, err := lisen.Accept()
		if err != nil {
			panic(err)
		}
		readbuff := make([]byte, 9, 9)
		con.Read(readbuff)
		conf, err := parseConf(readbuff)
		if err != nil {
			con.Close()
			panic(err)
		}
		fmt.Println(conf)
		con.Close()
	}

}
