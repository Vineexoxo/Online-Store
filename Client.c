#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"
#include <fcntl.h>
#include <sys/stat.h>

void displayProduct(struct product p);
void mainMenuUser();
void AdminMainMenu();

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv;

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
    {
        perror("Error: ");
        return -1;
    }

    printf("Success\n");
    printf("||                                                          ||\n");
    printf("||                 Welcome to our store, customer!          ||\n");
    printf("||                                                          ||\n");
    printf("\n");
        printf("  \x1b[34m[ 1 ]\x1b[0m  \x1b[1mLogin as user\x1b[0m                                                 \n");
        printf("  \x1b[34m[ 2 ]\x1b[0m  \x1b[1mLogin as admin\x1b[0m                                                \n");
    int who;
    printf("ENTER HERE: ");
    scanf("%d", &who);
    write(sockfd, &who, sizeof(who));

    if (who == 1)
    {
        printf("  \x1b[34m[ 1 ]\x1b[0m  \x1b[1mRegister as a user\x1b[0m                                                 \n");
        printf("  \x1b[34m[ 2 ]\x1b[0m  \x1b[1mAlready a user\x1b[0m                                                      \n");

        char user1;
        printf("ENTER HERE: ");
        scanf("%c", &user1);
        scanf("%c", &user1);
        if(user1 =='1'){
            char ch1='r';
            write(sockfd,&ch1,sizeof(char));
            int id;
            read(sockfd, &id, sizeof(int));
            printf("New user has been created.\n");
            printf("Your new customer id : %d\n\n", id);

        }

        while (1)
        {

            mainMenuUser();
            char ch;
            scanf("%c", &ch);
            scanf("%c", &ch);

            write(sockfd, &ch, sizeof(char));

            if (ch == '6')  //exit
            {
                break;
            }
            else if (ch == '1') //show products
            {   

                printf("+------------+----------------------+------------------+------------------+\n");
                printf("| ProductID  |     ProductName      |  Quantity |       Price      |\n");
                printf("+------------+----------------------+------------------+------------------+\n");

                while (1)
                {
                    struct product p;
                    read(sockfd, &p, sizeof(struct product));   //server returns products available in inventory
                    if (p.id != -1)
                    {
                        if (p.id != -1 && p.qty > 0)
                        {

                            printf("| %10d | %20s | %16d | %16.2d |\n", p.id, p.name, p.qty, p.price);
                        }
                    }
                    else    //if p.id=-1 break
                    {
                        break;
                    }
                }

                printf("+------------+----------------------+------------------+------------------+\n");
            }
            else if (ch == '2') //view cart
            {
                int customerID = -1;

                printf("Enter customer id\n");
                printf("ENTER HERE: ");
                scanf("%d", &customerID);

                write(sockfd, &customerID, sizeof(int));    //write cust_id on socket
                struct cart carti;
                read(sockfd, &carti, sizeof(struct cart));  //server returns the cart through the socket

                if (carti.custid != -1)
                {
                    printf("Customer ID: %d\n\n", carti.custid);
                    printf("+------------+----------------------+------------------+------------------+\n");
                    printf("| Product ID |     Product Name     | Quantity  |       Price      |\n");
                    printf("+------------+----------------------+------------------+------------------+\n");
                    for (int i = 0; i < 20; i++)    //we assuming cart limit has a max 20
                    {
                        if (carti.products[i].id != -1 && carti.products[i].qty > 0)   
                        {

                            printf("| %10d | %20s | %16d | %16.2d |\n", carti.products[i].id, carti.products[i].name, carti.products[i].qty, carti.products[i].price);
                        }
                    }
                    printf("+------------+----------------------+------------------+------------------+\n");
                }

                else
                {
                    printf("Wrong customerID provided\n");
                }
            }

            else if (ch == '3')// add products to your cart
            {
                int customerID = -1;

                printf("\n");
                printf("║       Enter Customer ID     ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &customerID);

                write(sockfd, &customerID, sizeof(int));    //write into the socket for the server to read

                int res;
                read(sockfd, &res, sizeof(int));    // server sends customer offset, if -1 error
                if (res == -1)
                {
                    printf("You have an empty cart!\n");
                    continue;
                }
                char reply[80];
                int product_id, qty;

                printf("\n");
                printf("║       Enter Product ID (int)     ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &product_id);


                printf("\n");
                printf("║       Enter Quantity        ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &qty);


                struct product p;
                p.id = product_id;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));  //send product id and quantity to the socket to server
                read(sockfd, reply, sizeof(reply));     //get the message of confirmation over the socket from server
                printf("%s", reply);
            }

            else if (ch == '4') //edit the cart
            {
                int customerID = -1;

                printf("\n");
                printf("║       Enter Customer ID      ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &customerID);

                write(sockfd, &customerID, sizeof(int));     //send customerID over the socket to server

                int res;
                read(sockfd, &res, sizeof(int));    //returns the customer offset over socket
                if (res == -1)
                {
                    printf("You have an empty cart\n");
                    continue;
                }

                int product_id, qty;

                printf("\n");
                printf("║       Enter Product ID (int)      ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &product_id);

                printf("Enter quantity\n");
                scanf("%d", &qty);

                struct product p;
                p.id = product_id;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));  //send the product to server over the socket

                char reply[80];
                read(sockfd, reply, sizeof(reply)); //server sends message of confirmation
                printf("%s", reply);
            }

            else if (ch == '5')
            {
                int customerID = -1;

                printf("\n");
                printf("║       Enter Customer ID     ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &customerID);

                write(sockfd, &customerID, sizeof(int));    //send customerID over the socket to server

                int res;
                read(sockfd, &res, sizeof(int));    //offset of customer
                if (res == -1)
                {
                    printf("Invalid customer id\n");
                    continue;
                }

                struct cart c;
                read(sockfd, &c, sizeof(struct cart));  //get cart from server

                int total = 0;  //calc total price
                for (int i = 0; i < 20; i++)
                {
                    if (c.products[i].id != -1)
                    {
                        total = total + c.products[i].qty * c.products[i].price;
                    }
                }

                printf("Value of your cart:");
                printf("%d\n", total);
                while (1)
                {
                    printf("Select your payment method:\n ");
                    printf("1) Credit Card\n");
                    printf("2) Debit Card\n");
                    printf("3) Cash\n");
                    printf("4) UPI\n");
                            printf("\x1b[1mMenu:\x1b[0m\n");
                    printf("  \x1b[34m[ 1 ]\x1b[0m  \x1b[1mCredit Card\x1b[0m                                                 \n");
                    printf("  \x1b[34m[ 2 ]\x1b[0m  \x1b[1mDebit Card\x1b[0m                                                      \n");
                    printf("  \x1b[34m[ 3 ]\x1b[0m  \x1b[1mCash\x1b[0m                                           \n");
                    printf("  \x1b[34m[ 4 ]\x1b[0m  \x1b[1mUPI\x1b[0m                                          \n");                    
                    printf("\n");
                    int choice;
                    scanf("%d", &choice);
                    if (choice > 4 || choice < 1)
                    {
                        printf("Invalid method, try again\n");
                    }
                    else
                    {
                        switch (choice)
                        {
                        case 1:
                            printf("You selected Credit Card payment method.\n");
                            printf("Enter Bank Name: ");
                            char bankName[100];
                            scanf(" %[^\n]", bankName);
                            printf("Enter Card Number: ");
                            char cardNumber[20];
                            scanf(" %[^\n]", cardNumber);
                            printf("Enter PIN: ");
                            int pin;
                            scanf("%d", &pin);

                            // Code for credit card payment processing

                            break;
                        case 2:
                            printf("You selected Debit Card payment method.\n");
                            printf("Enter Bank Name: ");
                            char bankNamee[100];
                            scanf(" %[^\n]", bankNamee);
                            printf("Enter Card Number: ");
                            char cardNumberr[20];
                            scanf(" %[^\n]", cardNumberr);
                            printf("Enter PIN: ");
                            int pinn;
                            scanf("%d", &pinn);

                            break;
                        case 3:
                            printf("You selected Cash payment method.\n");
                            // Add your code for Cash payment here
                            break;
                        case 4:
                            printf("You selected UPI payment method.\n");
                            printf("Enter UPI ID: ");
                            char upiID[100];
                            scanf(" %[^\n]", upiID);
                            printf("Enter UPI PIN: ");
                            int upiPIN;
                            scanf("%d", &upiPIN);

                            // Code for UPI payment processing
                            printf("Processing UPI payment...\n");
                            break;
                        }
                        break;
                    }
                }
                ch = 'y';
                printf("Payment Successfull!\n");
                write(sockfd, &ch, sizeof(char));   //confirmation
                read(sockfd, &ch, sizeof(char));
                write(sockfd, &total, sizeof(int));
                write(sockfd, &c, sizeof(struct cart));

                // Print the receipt data on the terminal
                printf("\n\n");
                printf("║          RECEIPT        ║\n");
                printf("\n");
                printf("║ Total:%-18d║\n", total);
                printf("\n");
                printf("║ Product ID ║  Quantity  ║  Price  ║\n");
                printf("\n");

                for (int i = 0; i < 20; i++)
                {
                    if (c.products[i].id == -1)
                    {
                        break;
                    }
                    printf("║ %-10d ║ %-10d ║ $%-6d ║\n", c.products[i].id, c.products[i].qty, c.products[i].price);
                }

                printf("\n");
            }

            else
            {
                printf("Invalid choice, try again\n");
            }
        }
    }
    else if (who == 2)
    {
        while (1)
        {
            AdminMainMenu();
            char ch;
            scanf("%c", &ch);
            scanf("%c", &ch);
            write(sockfd, &ch, sizeof(ch));
            if (ch == '1')  //see inventory
            {

                printf("+------------+----------------------+------------------+------------------+\n");
                printf("| ProductID  |     ProductName      |  Quantity |       Price      |\n");
                printf("+------------+----------------------+------------------+------------------+\n");

                while (1)
                {
                    struct product p;
                    read(sockfd, &p, sizeof(struct product));
                    if (p.id != -1)
                    {
                        if (p.id != -1 && p.qty > 0)
                        {

                            printf("| %10d | %20s | %16d | %16.2d |\n", p.id, p.name, p.qty, p.price);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                printf("+------------+----------------------+------------------+------------------+\n");
            }
            else if (ch == '2')// add a product
            {
                
                int product_id, qty;
                char name[50];

                printf("\n");
                printf("║       Enter Product Name (1 string)      ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%s", name);

                printf("\n");
                printf("║       Enter Product ID (int)     ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &product_id);


                printf("\n\n");
                printf("║     Enter Quantity (+ve)    ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &qty);


                int price = -1;

                printf("\n");
                printf("║       Enter Price ID        ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &price);

                struct product p;
                p.id = product_id;
                strcpy(p.name, name);
                p.qty = qty;
                p.price = price;

                 write(sockfd, &p, sizeof(struct product)); //send products over the socket to server

                char reply[80];
                int n = read(sockfd, reply, sizeof(reply));
                reply[n] = '\0';

                printf("%s", reply);
            }

            else if (ch == '3') //delete a product
            {
                //deleting is equivalent to setting everything as -1

                int id = -1;

                printf("\n");
                printf("║       Enter Product ID (int)     ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &id);

                write(sockfd, &id, sizeof(int));

                char reply[80];
                read(sockfd, reply, sizeof(reply)); //MESSAGE OF confirmation
                printf("%s\n", reply);
            }

            else if (ch == '4') //update price
            {
                int id = -1;

                printf("\n");
                printf("║       Enter Product ID (int)      ║\n");
                printf("\n");
                printf("            ");
                scanf("%d", &id);

                int price = -1;
                printf("\n");
                printf("║       Enter New Price           ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &price);

                struct product p;
                p.id = id;
                p.price = price;
                write(sockfd, &p, sizeof(struct product));

                char reply[80];
                read(sockfd, reply, sizeof(reply));
                printf("%s\n", reply);
            }

            else if (ch == '5')
            {
                int id = -1;

                printf("\n");
                printf("║       Enter Product ID (int)      ║\n");
                printf("\n");
                printf("ENTER HERE: ");
                scanf("%d", &id);

                int qty = -1;

                printf("\n");
                printf("║     Enter Quantity (+ve)    ║\n");
                printf("\n");
                printf("            ");
                scanf("%d", &qty);



                struct product p;
                p.id = id;
                p.qty = qty;
                write(sockfd, &p, sizeof(struct product));

                char reply[80];
                read(sockfd, reply, sizeof(reply));
                printf("%s\n", reply);
            }
            else if (ch == '6') //exit
            {
                break;
            }

            else
            {
                printf("Wrong choice, try again\n");
            }
        }
    }

    printf("Visit Again!\n");
    close(sockfd);
    return 0;
}

void mainMenuUser(){

    printf("||                                                          ||\n");
    printf("||                 Welcome to our store, customer!          ||\n");
    printf("||                                                          ||\n \n");

        printf("\x1b[1mMenu:\x1b[0m\n");
        printf("  \x1b[34m[ 1 ]\x1b[0m  \x1b[1mList availabe items\x1b[0m                                                 \n");
        printf("  \x1b[34m[ 2 ]\x1b[0m  \x1b[1mView your cart\x1b[0m                                                      \n");
        printf("  \x1b[34m[ 3 ]\x1b[0m  \x1b[1mAdd products to your cart\x1b[0m                                           \n");
        printf("  \x1b[34m[ 4 ]\x1b[0m  \x1b[1mEdit products in your cart\x1b[0m                                          \n");
        printf("  \x1b[34m[ 5 ]\x1b[0m  \x1b[1mProceed to checkout\x1b[0m                                                 \n");
        printf("  \x1b[34m[ 6 ]\x1b[0m  \x1b[1mExit Menu\x1b[0m                                                           \n");
        printf("\n");

        printf("ENTER HERE:");

}
void displayProduct(struct product p)
{
    if (p.id != -1 && p.qty > 0)
    {
        printf("| %10d | %20s | %16d | %16.2d |\n", p.id, p.name, p.qty, p.price);
    }
}
void AdminMainMenu(){
            printf("\n------------------\n");
            printf("\x1b[1mMenu to choose from:\x1b[0m\n");
            printf("\x1b[33m1.\x1b[0m See inventory\n");
            printf("\x1b[33m2.\x1b[0m Add a product\n");
            printf("\x1b[33m3.\x1b[0m Delete a product\n");
            printf("\x1b[33m4.\x1b[0m Update the price of an existing product\n");
            printf("\x1b[33m5.\x1b[0m Update the quantity of an existing product\n");
            printf("\x1b[33m6.\x1b[0m Exit Shop\n");
            printf("Please enter your choice\n");
}