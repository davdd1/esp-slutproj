package main

import ( 
	"fmt"
	"go-server/internal"
)

func main() {
	// Start tcp-server
	fmt.Println("Starting TCP server...")
	internal.StartServer("0.0.0.0:8080") 
}