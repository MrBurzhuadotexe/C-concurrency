# IPC
#### Sleeping barber solution
The task is a classic example of [sleeping barber problem](https://en.wikipedia.org/wiki/Sleeping_barber_problem) with extra steps, that include:
1. The waiting room (implemented using message queue).
2. The client's settlement with the barber:
   - the client gives the hairdresser the amount for the service before shaving;
   - the barber gives the change after completing customer service, and if the customer cannot give the change, the customer must wait until the barber finds the change at the cash register.
3. The cash register, which is common to all barbers.
4. Payments can be made in denominations of 1, 2, and 5.


#### The barber goes through the loop:
1. select a client from the waiting room (or sleep if he is not there yet)
2. find a free chair
3. collect a fee for the service and places it in a common cash register
4. provide the service
5. free the chair
6. calculate the change and take it from the common cash register, and if this is not possible, wait until the appropriate denominations appear as a result of the work of another barber,
7. give the change to the customer

#### The client goes through the loop:
1. earn money
2. come to the barbershop,
3. if there is a free place in the waiting room, sit down and wait for the service (or wake up the barber), and if there are no places left, leave and return to earning money,
4. after finding a barber, pay for the haircut
5. wait until the service is done
6. wait for the change
7. leave the barbershop and go back to making money.



