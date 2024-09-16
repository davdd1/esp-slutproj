package internal

import (
	"fmt"
	"net"
	"strconv"
	"time"
)

const timeoutDuration = 500 * time.Second // Set a timeout of 5 seconds for reading/writing

// StartServer starts a TCP server and listens on the provided address.
func StartServer(address string) {
	listener, err := net.Listen("tcp", address)
	if err != nil {
		fmt.Println("\033[31m[✘] Error starting server:\033[0m", err) // Red for errors
		return
	}
	defer listener.Close()

	fmt.Println("\033[32m[✔] Server listening on\033[0m", address) // Green for successful server start

	for {
		conn, err := listener.Accept()
		if err != nil {
			fmt.Println("\033[31m[✘] Error accepting connection:\033[0m", err) // Red for connection errors
			continue
		}
		go handleConnection(conn)
	}
}

// handleConnection handles the incoming connection and processes messages.
func handleConnection(conn net.Conn) {
	defer conn.Close()
	buffer := make([]byte, 1024)

	for {
		fmt.Println("\033[33m[~] Waiting for data from client...\033[0m") // Yellow for waiting

		// Set a read deadline (timeout) for the connection
		conn.SetReadDeadline(time.Now().Add(timeoutDuration))

		n, err := conn.Read(buffer)
		if err != nil {
			if netErr, ok := err.(net.Error); ok && netErr.Timeout() {
				fmt.Println("\033[31m[✘] Read timeout, closing connection.\033[0m") // Red for timeout errors
			} else {
				fmt.Println("\033[31m[✘] Connection closed or error:\033[0m", err) // Red for generic connection errors
			}
			return
		}

		message := string(buffer[:n])

		if len(message) > 0 {
			switch message[0] {
			case '1':
				macAddress := message[1:]
				fmt.Println("\033[36m[↓] Received:\033[0m " + message + "\n\033[36m[ℹ] Handling MAC address!") // Cyan for info
				handleMACAddress(macAddress, conn)
			case '2':
				temperature := message[1:]
				fmt.Println("\033[36m[↓] Received:\033[0m " + message + "\n\033[36m[ℹ] Handling temperature!") // Cyan for info
				handleTemperature(temperature, conn)
			// case '4': // Keep-alive message (if implemented)
			// 	fmt.Println("\033[36m[ℹ] Received keep-alive message\033[0m")
			default:
				fmt.Println("\033[31m[✘] Unknown message type\033[0m") // Red for unknown types
			}
		} else {
			fmt.Println("\033[33m[!] Empty message received.\033[0m") // Yellow for empty messages
		}

		fmt.Println("\033[32m[↑] Sending acknowledgment to client...\033[0m") // Green for acknowledgment
		conn.Write([]byte("Acknowledged: " + message))
	}
}

// handleMACAddress processes the MAC address message from the client.
func handleMACAddress(mac string, conn net.Conn) {
	if IsKnownDevice(mac) {
		// Send WELCOME BACK message for known devices
		fmt.Println("\033[34m[✔] Known device!:\033[0m", mac) // Blue for known devices
		conn.Write([]byte("Welcome back! Device: " + mac + "\n")) // Send welcome back message
	} else {
		// Register the new device and send a welcome message
		RegisterDevice(mac)
		fmt.Println("\033[32m[✔] New device!:\033[0m", mac) // Green for new devices
		conn.Write([]byte("Welcome! Device: " + mac + "\n")) // Send welcome message
	}
}

// handleTemperature processes the temperature message from the client.
func handleTemperature(temp string, conn net.Conn) {
	//convert temperature to float
	temperature, err := strconv.ParseFloat(temp, 64)
	if err != nil {
		fmt.Println("\033[31m[✘] Error parsing temperature:\033[0m", err)// Red for parsing errors
		return
	}

	fmt.Println("\033[35m[ℹ] Processing temperature:\033[0m", temp+" °C") // Magenta for temperature

	// Check the temperature and determine a color
	var ledCommand string
	if temperature >= 25 {
		ledCommand = "LED:255,0,0" // Red
		fmt.Println("\033[31m[🔥] High temperature detected!\033[0m") // Red for high temperature
	} else if temperature < 22 {
		ledCommand = "LED:0,0,255" // Blue
		fmt.Println("\033[34m[❄️] Cold temperature detected.\033[0m") // Blue for cold temperature
	} else {
		ledCommand = "LED:0,255,0" // Green
		fmt.Println("\033[32m[🌡️] Normal temperature.\033[0m") // Green for normal temperature
	}

	// Send the LED command to the hub
	conn.Write([]byte(ledCommand))
}
