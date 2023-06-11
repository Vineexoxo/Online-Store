# OnlineStore IMT2021018

An online store implemented using system calls and Linux programming in C as a part of the Operating Systems cou# Online Shopping System

This is an online shopping system that allows customers to browse products, add them to their shopping cart, and place orders. The system consists of a client-server architecture where clients interact with the server to perform various operations.

## Features

- *Product Management*: The server provides functionalities for adding, deleting, and modifying product details such as ID, name, quantity, and price.

- *Shopping Cart*: Customers can add products to their shopping carts, view the contents, and modify the quantities.

- *Order Placement*: Customers can place orders, which are stored in a separate file along with the customer details.

- *Concurrency and Synchronization*: The system handles concurrent access to files using file locks to ensure data integrity and avoid conflicts.

# Prerequisites

- C compiler
- Linux operating system

# Acknowledgements

- The project was developed as part of the Operating Systems course.
- Special thanks to the course instructor and peers for their guidance and support.
# Program Architecture
1. There are 2 programs - Server.c and Client.c. Server.c is the Server code, while Client.c is the Client code, which allows you to login as a user or as the admin.
2. The program uses sockets to communicate between the server and client, and file locking while accessing the files which contain data.
3. customers.txt contains the list of customers registered with the program, orders.txt contains the cart for each customer, records.txt contains the products in the inventory, and receipt.txt contains the recept generated once payment for a customer is completed.

# Instructions to run the program
Open a terminal, and run the following commands 

```
gcc -o server Server.c
./server
```

In a separate terminal, run the following commands
```
gcc -o client Client.c
./client
```

Now you can use the user menu or admin menu as directed by the program to perform operations on the products or customers.

Kindly refer to the project report for details on implementation.
