#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "headers.h"

void unlock(int fd_inventory, struct flock lock);
void readLock(int fd_inventory, struct flock lock);
void CartLock(int fd_cart, struct flock lock_cart, int offset, int ch);
int getOffset(int cust_id, int fd_custs);
void RegisterUser(int fd_custs,int fd_cart,int new_socketDescriptor);
int customerOffset(int new_socketDescriptor,int fd_custs);

int main()
{

    // file containing all the records is called records.txt

    int fd_inventory = open("inventory.txt", O_RDWR | O_CREAT, 0777);     //where all the procucts are stored.
    int fd_cart = open("orders.txt", O_RDWR | O_CREAT, 0777);   //where all the carts are stored.
    int fd_custs = open("customer.txt", O_RDWR | O_CREAT, 0777);    //where all the customers are stored.
    int fd_admin = open("loggingAdmin.txt", O_RDWR | O_CREAT, 0777); //logs for the admin to keep track of
    int fd_bill = open("Bill.txt", O_CREAT | O_RDWR, 0777); //transaction
    lseek(fd_admin, 0, SEEK_END);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in server1, client1;

    server1.sin_family = AF_INET;
    server1.sin_addr.s_addr = INADDR_ANY;
    server1.sin_port = htons(5555);

    int opt = 1;    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("Error: ");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&server1, sizeof(server1)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    if (listen(sockfd, 5) == -1)
    {
        perror("Error: ");
        return -1;
    }

    int size = sizeof(client1);
    printf("Store is Online\n");

    while (1)
    {

        int new_socketDescriptor = accept(sockfd, (struct sockaddr *)&client1, &size);  //accept the incoming calls from the socket
        if (new_socketDescriptor == -1)
        {
            return -1;
        }

        if (!fork())
        {
            printf("Connection with client successful\n");
            close(sockfd);

            int user;
            read(new_socketDescriptor, &user, sizeof(int));     //user stores input from client end which says whether its user or admin

            if (user == 1)  //user
            {

                char ch;
                while (1)
                {
                    read(new_socketDescriptor, &ch, sizeof(char));

                    lseek(fd_inventory, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);

                    if (ch == '6')  //client exits the server by closing the socket
                    {
                        printf("Connection terminated\n");
                        close(new_socketDescriptor);
                        break;
                    }
                    else if (ch == '1') //show all the products in inventory
                    {

                        struct flock lock;  
                        readLock(fd_inventory, lock);

                        struct product p;
                        while (read(fd_inventory, &p, sizeof(struct product)))    //read from inventory.txt
                        {
                            if (p.id != -1)
                            {
                                write(new_socketDescriptor, &p, sizeof(struct product));    //send all valid products with id>-1
                            }
                        }

                        p.id = -1;
                        write(new_socketDescriptor, &p, sizeof(struct product));    // for the iteration to stop at client end
                        unlock(fd_inventory, lock);
                    }

                    else if (ch == '2')     //client asks for cart
                    {

                        int offset = customerOffset(new_socketDescriptor,fd_custs);
                        struct cart c;

                        if (offset == -1)   //invalid cust_id (<0)
                        {

                            struct cart c;
                            c.custid = -1;
                            write(new_socketDescriptor, &c, sizeof(struct cart));   //send an empty cart
                        }
                        else
                        {
                            struct cart c;
                            struct flock lock_cart;

                            CartLock(fd_cart, lock_cart, offset, 1);    //locks the cart at offset
                            read(fd_cart, &c, sizeof(struct cart)); //read from cart
                            write(new_socketDescriptor, &c, sizeof(struct cart)); //write it on the socket to the client
                            unlock(fd_cart, lock_cart); //unlock
                        }
                    }

                    else if (ch == '3') // add products to cart
                    {
                        int offset = customerOffset(new_socketDescriptor,fd_custs);  // get the offset of customer corresponding to cus_id

                        write(new_socketDescriptor, &offset, sizeof(int));  //send the customer offset over the socket

                        if (offset == -1)
                        {
                            continue;
                        }

                        struct flock lock_cart;

                        int i = -1;
                        CartLock(fd_cart, lock_cart, offset, 1);    //lock the cart corresponding to offset
                        struct cart carti;
                        read(fd_cart, &carti, sizeof(struct cart)); //from the fd_cart file descriptor

                        struct flock lock_product;
                        readLock(fd_inventory, lock_product);

                        struct product p;
                        read(new_socketDescriptor, &p, sizeof(struct product)); //the product user wants to add to his cart through socket
                        struct product p1;
                        int found = 0;
                        while (read(fd_inventory, &p1, sizeof(struct product)))   //get the product
                        {
                            if (p1.id == p.id)  //check id
                            {
                                if (p1.qty >= p.qty)
                                {
                                    found = 1;
                                    break;
                                }
                            }
                        }
                        unlock(fd_cart, lock_cart);
                        unlock(fd_inventory, lock_product);   //unlock all locks

                        if (!found)
                        {
                            write(new_socketDescriptor, "Product is Unavailable\n", sizeof("Product is Unavailable\n"));
                            continue;
                        }

                        int flg = 0;    // flag to check if cart is full

                        int flg1 = 0;   //flag to check if product available
                        for (int i = 0; i < 20; i++)        //if product exists in cart
                        {
                            if (carti.products[i].id == p.id)
                            {
                                flg1 = 1;
                                break;
                            }
                        }

                        if (flg1)
                        {
                            write(new_socketDescriptor, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
                            continue;
                        }

                        for (int i = 0; i < 20; i++)
                        {
                            if (carti.products[i].id == -1)
                            {
                                flg = 1;
                                carti.products[i].id = p.id;
                                carti.products[i].qty = p.qty;
                                strcpy(carti.products[i].name, p1.name);
                                carti.products[i].price = p1.price;
                                break;
                            }
                            else if (carti.products[i].qty <= 0)
                            {
                                flg = 1;
                                carti.products[i].id = p.id;
                                carti.products[i].qty = p.qty;
                                strcpy(carti.products[i].name, p1.name);
                                carti.products[i].price = p1.price;
                                break;
                            }
                        }

                        if (!flg)
                        {
                            write(new_socketDescriptor, "Cart limit reached\n", sizeof("Cart limit reached\n"));    //send over the socket
                            continue;
                        }

                        write(new_socketDescriptor, "Item added to cart\n", sizeof("Item added to cart\n"));    //over the socket to user

                        CartLock(fd_cart, lock_cart, offset, 2);    //lock the cart at offset so no edits at the same time
                        write(fd_cart, &carti, sizeof(struct cart));    //write cart in the cart.txt
                        unlock(fd_cart, lock_cart); //unlock cart
                    }

                    else if (ch == '4'){    //edit the cart

                        int offset = customerOffset( new_socketDescriptor, fd_custs);   //get the offset of customer corresponding to user cust_id

                        write(new_socketDescriptor, &offset, sizeof(int));  //giving back the offset over the socket to user
                        if (offset == -1)
                        {
                            continue;
                        }

                        struct flock lock_cart;
                        CartLock(fd_cart, lock_cart, offset, 1);    //lock the cart at the offset
                        struct cart c;
                        read(fd_cart, &c, sizeof(struct cart)); //get the cart

                        int product_id, qty;
                        struct product p;
                        read(new_socketDescriptor, &p, sizeof(struct product)); //get product info from user to be updated/edited

                        product_id = p.id;
                        qty = p.qty;

                        int flg = 0;    //flag to check if product available
                        int i;
                        for (i = 0; i < 20; i++)
                        {
                            if (c.products[i].id == product_id)
                            {

                                struct flock lock_product;
                                readLock(fd_inventory, lock_product);

                                struct product p1;
                                while (read(fd_inventory, &p1, sizeof(struct product)))
                                {
                                    if (p1.id == product_id && p1.qty > 0)
                                    {
                                        if (p1.qty >= qty)
                                        {
                                            flg = 1;
                                            break;
                                        }
                                        else
                                        {
                                            flg = 0;
                                            break;
                                        }
                                    }
                                }
                                unlock(fd_inventory, lock_product);
                                break;
                            }
                        }
                        unlock(fd_cart, lock_cart);

                        if (!flg)
                        {
                            write(new_socketDescriptor, "Out Of stock!\n", sizeof("Out of stock!\n"));
                            continue;
                        }

                        c.products[i].qty = qty;
                        write(new_socketDescriptor, "Update successful\n", sizeof("Update successful\n"));
                        CartLock(fd_cart, lock_cart, offset, 2);    //write lock cart so no edits at the same time
                        write(fd_cart, &c, sizeof(struct cart)); //send info over the 
                        unlock(fd_cart, lock_cart); //unlock cart
                    }

                    else if (ch == '5')
                    {
                        int offset =customerOffset(new_socketDescriptor,fd_custs);  //get offset of customer

                        write(new_socketDescriptor, &offset, sizeof(int));  //send offset to client
                        if (offset == -1)
                        {
                            continue;
                        }

                        struct flock lock_cart;
                        CartLock(fd_cart, lock_cart, offset, 1);    //read lock the cart

                        struct cart c;
                        read(fd_cart, &c, sizeof(struct cart)); //get cart from orders.txt
                        unlock(fd_cart, lock_cart); //unlock cart
                        write(new_socketDescriptor, &c, sizeof(struct cart));   //send cart over the socket

                        int total = 0;

                        for (int i = 0; i < 20; i++)
                        {

                            if (c.products[i].id != -1) //product available
                            {
                                write(new_socketDescriptor, &c.products[i].qty, sizeof(int));   //send product quantity

                                struct flock lock_product;
                                readLock(fd_inventory, lock_product);
                                lseek(fd_inventory, 0, SEEK_SET);

                                struct product p;
                                int flg = 0;
                                while (read(fd_inventory, &p, sizeof(struct product)))
                                {

                                    if (p.id == c.products[i].id && p.qty > 0)
                                    {
                                        int min;
                                        if (c.products[i].qty >= p.qty)
                                        {
                                            min = p.qty;
                                        }
                                        else
                                        {
                                            min = c.products[i].qty;
                                        }

                                        flg = 1;
                                        write(new_socketDescriptor, &min, sizeof(int));
                                        write(new_socketDescriptor, &p.price, sizeof(int));
                                        break;
                                    }
                                }

                                unlock(fd_inventory, lock_product);

                                if (!flg)
                                {
                                    // product got deleted midway
                                    int val = 0;
                                    write(new_socketDescriptor, &val, sizeof(int));
                                    write(new_socketDescriptor, &val, sizeof(int));
                                }
                            }
                        }

                        char ch;
                        read(new_socketDescriptor, &ch, sizeof(char));

                        for (int i = 0; i < 20; i++)
                        {

                            struct flock lock_product;
                            readLock(fd_inventory, lock_product);
                            lseek(fd_inventory, 0, SEEK_SET);

                            struct product p;
                            while (read(fd_inventory, &p, sizeof(struct product)))
                            {

                                if (p.id == c.products[i].id)
                                {
                                    int min;
                                    if (c.products[i].qty >= p.qty)
                                    {
                                        min = p.qty;
                                    }
                                    else
                                    {
                                        min = c.products[i].qty;
                                    }
                                    unlock(fd_inventory, lock_product);
                                    // product write lock
                                    lseek(fd_inventory, (-1) * sizeof(struct product), SEEK_CUR);
                                    lock_product.l_type = F_WRLCK;
                                    lock_product.l_whence = SEEK_CUR;
                                    lock_product.l_start = 0;
                                    lock_product.l_len = sizeof(struct product);
                                    fcntl(fd_inventory, F_SETLKW, &lock_product);
                                    p.qty = p.qty - min;

                                    write(fd_inventory, &p, sizeof(struct product));
                                    unlock(fd_inventory, lock_product);
                                }
                            }

                            unlock(fd_inventory, lock_product);
                        }

                        CartLock(fd_cart, lock_cart, offset, 2);

                        for (int i = 0; i < 20; i++)
                        {
                            c.products[i].id = -1;
                            strcpy(c.products[i].name, "");
                            c.products[i].price = -1;
                            c.products[i].qty = -1;
                        }

                        write(fd_cart, &c, sizeof(struct cart));
                        write(new_socketDescriptor, &ch, sizeof(char));
                        unlock(fd_cart, lock_cart);

                        read(new_socketDescriptor, &total, sizeof(int));
                        read(new_socketDescriptor, &c, sizeof(struct cart));


                        write(fd_bill, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));
                        char temp[100];
                        for (int i = 0; i < 20; i++)
                        {
                            if (c.products[i].id != -1)
                            {
                                sprintf(temp, "%d\t%s\t%d\t%d\n", c.products[i].id, c.products[i].name, c.products[i].qty, c.products[i].price);
                                write(fd_bill, temp, strlen(temp));
                            }
                        }
                        sprintf(temp, "Total - %d\n", total);
                        write(fd_bill, temp, strlen(temp));
                        close(fd_bill);
                    }

                    else if (ch == 'r')
                    {
                        RegisterUser(fd_custs,fd_cart,new_socketDescriptor);
                    }
                }
            }
            else if (user == 2) //admin
            {
                char ch;
                while (1)
                {
                    read(new_socketDescriptor, &ch, sizeof(ch));    //choice entered by user

                    lseek(fd_inventory, 0, SEEK_SET);
                    lseek(fd_cart, 0, SEEK_SET);
                    lseek(fd_custs, 0, SEEK_SET);
                    if (ch == '1')  //show inventory
                    {
                        struct flock lock;
                        readLock(fd_inventory, lock); //lock inventory

                        struct product p;
                        while (read(fd_inventory, &p, sizeof(struct product)))  //read all products from inventory
                        {
                            if (p.id != -1)
                            {
                                write(new_socketDescriptor, &p, sizeof(struct product));    //send to the client over the socket
                            }
                        }

                        p.id = -1;  //to let the client know this marks the end of products
                        write(new_socketDescriptor, &p, sizeof(struct product));    
                        unlock(fd_inventory, lock); //unlock inventory
                    }

                    if (ch == '2')  //add a product
                    {
                        char name[50];
                        char response[100];
                        int product_id, qty, price;

                        struct product p1;
                        int n = read(new_socketDescriptor, &p1, sizeof(struct product));

                        strcpy(name, p1.name);
                        product_id = p1.id;
                        qty = p1.qty;
                        price = p1.price;

                        struct flock lock;

                        readLock(fd_inventory, lock);

                        struct product p;

                        int flg = 0;    //flag to check for duplication
                        while (read(fd_inventory, &p, sizeof(struct product)))  //iterate through all the products in the inventory
                        {

                            if (p.id == product_id && p.qty > 0)
                            {
                                write(new_socketDescriptor, "Duplicate product\n", sizeof("Duplicate product\n"));
                                sprintf(response, "Adding product with product id %d failed.\n", product_id);
                                write(fd_admin, response, strlen(response));    //logs stored for admin
                                unlock(fd_inventory, lock);
                                flg = 1;
                                break;
                            }
                        }

                        if (!flg)
                        {

                            lseek(fd_inventory, 0, SEEK_END);
                            p.id = product_id;
                            strcpy(p.name, name);
                            p.price = price;
                            p.qty = qty;

                            write(fd_inventory, &p, sizeof(struct product));
                            write(new_socketDescriptor, "Added successfully\n", sizeof("Added succesfully\n"));
                            sprintf(response, "Product with product id %d added successfully\n", product_id);
                            write(fd_admin, response, strlen(response));    //log stored for admin
                            unlock(fd_inventory, lock);
                        }
                    }
                    else if (ch == '3') //delete a product
                    {
                        
                        int product_id;
                        read(new_socketDescriptor, &product_id, sizeof(int));

                        struct flock lock;
                        readLock(fd_inventory, lock);
                        char response[100];

                        struct product p;
                        int flg = 0;
                        while (read(fd_inventory, &p, sizeof(struct product)))
                        {
                            if (p.id == product_id)
                            {

                                unlock(fd_inventory, lock);
                                // product write lock
                                lseek(fd_inventory, (-1) * sizeof(struct product), SEEK_CUR);
                                lock.l_type = F_WRLCK;
                                lock.l_whence = SEEK_CUR;
                                lock.l_start = 0;
                                lock.l_len = sizeof(struct product);

                                fcntl(fd_inventory, F_SETLKW, &lock);

                                p.id = -1;  //invalidate by setting -1
                                strcpy(p.name, "");
                                p.price = -1;
                                p.qty = -1;

                                write(fd_inventory, &p, sizeof(struct product));
                                write(new_socketDescriptor, "Delete successful", sizeof("Delete successful"));
                                sprintf(response, "Product with product id %d deleted succesfully\n", product_id);
                                write(fd_admin, response, strlen(response));

                                unlock(fd_inventory, lock);
                                flg = 1;
                                break;
                            }
                        }

                        if (!flg)
                        {
                            sprintf(response, "ProductID=%d does not exist, deletion failed.\n", product_id);
                            write(fd_admin, response, strlen(response));    //logs for admin
                            write(new_socketDescriptor, "Product id invalid", sizeof("Product id invalid"));
                            unlock(fd_inventory, lock);
                        }
                    }
                    else if (ch == '4') //update price
                    {
                        int id;
                        int val = -1;
                        struct product p1;
                        read(new_socketDescriptor, &p1, sizeof(struct product));
                        id = p1.id;
                        char response[100];

                        val = p1.price;

                        struct flock lock;
                        readLock(fd_inventory, lock);

                        int flg = 0;

                        struct product p;
                        while (read(fd_inventory, &p, sizeof(struct product)))
                        {
                            if (p.id == id)
                            {

                                unlock(fd_inventory, lock);
                                // product write lock
                                lseek(fd_inventory, (-1) * sizeof(struct product), SEEK_CUR);
                                lock.l_type = F_WRLCK;
                                lock.l_whence = SEEK_CUR;
                                lock.l_start = 0;
                                lock.l_len = sizeof(struct product);

                                fcntl(fd_inventory, F_SETLKW, &lock);
                                int old;

                                old = p.price;
                                p.price = val;

                                write(fd_inventory, &p, sizeof(struct product));

                                write(new_socketDescriptor, "The price of the product has been successfully modified.", sizeof("The price of the product has been successfully modified."));

                                sprintf(response, "Price of product with ID %d has been successfully modified from %d to %d.\n", id, old, val);
                                write(fd_admin, response, strlen(response));

                                unlock(fd_inventory, lock);
                                flg = 1;
                                break;
                            }
                        }

                        if (!flg)
                        {
                            write(new_socketDescriptor, "Product id invalid", sizeof("Product id invalid"));
                            unlock(fd_inventory, lock);
                        }
                    }

                    else if (ch == '5') //update quantity
                    {
                        int id;
                        int val = -1;
                        struct product p1;
                        read(new_socketDescriptor, &p1, sizeof(struct product));
                        id = p1.id;

                        char response[100];

                        val = p1.qty;

                        struct flock lock;
                        readLock(fd_inventory, lock);

                        int flg = 0;

                        struct product p;
                        while (read(fd_inventory, &p, sizeof(struct product)))
                        {
                            if (p.id == id)
                            {

                                unlock(fd_inventory, lock);
                                // product write lock
                                lseek(fd_inventory, (-1) * sizeof(struct product), SEEK_CUR);
                                lock.l_type = F_WRLCK;
                                lock.l_whence = SEEK_CUR;
                                lock.l_start = 0;
                                lock.l_len = sizeof(struct product);

                                fcntl(fd_inventory, F_SETLKW, &lock);
                                int old;

                                old = p.qty;
                                p.qty = val;

                                write(fd_inventory, &p, sizeof(struct product));

                                sprintf(response, "Quantity of product with product id %d modified from %d to %d \n", id, old, val);
                                write(fd_admin, response, strlen(response));
                                write(new_socketDescriptor, "Quantity modified successfully", sizeof("Quantity modified successfully"));

                                unlock(fd_inventory, lock);
                                flg = 1;
                                break;
                            }
                        }

                        if (!flg)
                        {
                            write(new_socketDescriptor, "Product id invalid", sizeof("Product id invalid"));
                            unlock(fd_inventory, lock);
                        }
                    }

                    else if (ch == '6')
                    {
                        printf("Connection terminated\n");
                        close(new_socketDescriptor);
                        struct flock lock;
                        readLock(fd_inventory, lock);
                        write(fd_admin, "Current Inventory:\n", strlen("Current Inventory:\n"));
                        write(fd_admin, "ProductID\tProductName\tQuantity\tPrice\n", strlen("ProductID\tProductName\tQuantity\tPrice\n"));

                        lseek(fd_inventory, 0, SEEK_SET);
                        struct product p;
                        while (read(fd_inventory, &p, sizeof(struct product)))
                        {
                            if (p.id != -1)
                            {
                                char temp[100];
                                sprintf(temp, "%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
                                write(fd_admin, temp, strlen(temp));
                            }
                        }
                        unlock(fd_inventory, lock);
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            // printf("Connection terminated\n");
        }
        else
        {
            close(new_socketDescriptor);
        }
    }
}

void unlock(int fd_inventory, struct flock lock)
{
    lock.l_type = F_UNLCK;
    fcntl(fd_inventory, F_SETLKW, &lock);
}

void readLock(int fd_inventory, struct flock lock)
{
    lock.l_len = 0;
    lock.l_type = F_RDLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    fcntl(fd_inventory, F_SETLKW, &lock);
}

void CartLock(int fd_cart, struct flock lock_cart, int offset, int ch)
{
    lock_cart.l_whence = SEEK_SET;
    lock_cart.l_len = sizeof(struct cart);
    lock_cart.l_start = offset;
    if (ch == 1)
    {
        // rdlck
        lock_cart.l_type = F_RDLCK;
    }
    else
    {
        // wrlck
        lock_cart.l_type = F_WRLCK;
    }
    fcntl(fd_cart, F_SETLKW, &lock_cart);
    lseek(fd_cart, offset, SEEK_SET);
}

int getOffset(int cust_id, int fd_custs)
{
    if (cust_id < 0)
    {
        return -1;
    }
    struct flock lock_cust;
    lock_cust.l_len = 0;    //lock customer
    lock_cust.l_type = F_RDLCK;
    lock_cust.l_start = 0;
    lock_cust.l_whence = SEEK_SET;
    fcntl(fd_custs, F_SETLKW, &lock_cust);
    struct index id;

    while (read(fd_custs, &id, sizeof(struct index)))
    {
        if (id.custid == cust_id)
        {
            unlock(fd_custs, lock_cust);
            return id.offset;
        }
    }
    unlock(fd_custs, lock_cust);
    return -1;
}
void RegisterUser(int fd_custs,int fd_cart,int new_socketDescriptor){
                            // char buf;
                        // read(new_socketDescriptor, &buf, sizeof(char));

                        struct flock lock;
                        lock.l_len = 0;
                        lock.l_type = F_RDLCK;
                        lock.l_start = 0;
                        lock.l_whence = SEEK_SET;
                        fcntl(fd_custs, F_SETLKW, &lock);

                        int max_id = -1;
                        struct index id;
                        while (read(fd_custs, &id, sizeof(struct index)))
                        {
                            if (id.custid > max_id)
                            {
                                max_id = id.custid;
                            }
                        }

                        max_id++;

                        id.custid = max_id;
                        id.offset = lseek(fd_cart, 0, SEEK_END);
                        lseek(fd_custs, 0, SEEK_END);
                        write(fd_custs, &id, sizeof(struct index));

                        struct cart c;
                        c.custid = max_id;
                        for (int i = 0; i < 20; i++)
                        {
                            c.products[i].id = -1;
                            strcpy(c.products[i].name, "");
                            c.products[i].qty = -1;
                            c.products[i].price = -1;
                        }
                        lseek(fd_cart, 0, SEEK_END);
                        write(fd_cart, &c, sizeof(struct cart));
                        unlock(fd_custs, lock);
                        write(new_socketDescriptor, &max_id, sizeof(int));
}
int customerOffset(int new_socketDescriptor,int fd_custs){
                        int cust_id = -1;
                        read(new_socketDescriptor, &cust_id, sizeof(int));  //read cust_id coming from client end
                        int offset = getOffset(cust_id, fd_custs);  //offset of customer
                        return offset;
}