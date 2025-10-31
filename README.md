# Banking Management System

This is a simple Banking Management System implemented in C using **file handling**, **socket programming**, **file locking** (`flock`), and **concurrency** (`pthread`). The system uses a client-server architecture to allow multiple users to perform various banking operations concurrently.

The system supports four distinct user roles: **Admin**, **Manager**, **Employee**, and **Customer**, each with specific functionalities and permissions.

---

## 🚀 Features

The system includes the following roles:

### 🧑‍💼 Admin

- **Login:** Admin can log in using predefined credentials.
- **View All Customers:** Admin can view a list of all customers in the system along with their details.
- **Add New Customer:** Admin can add a new customer by providing a name, password, and initial balance.
- **Remove Customer:** Admin can remove a customer from the system by entering their ID.
- **Manage User Roles:** Add or remove Employees and Managers from the system.
- **Modify User Details:** Update credentials for Customers, Employees, and Managers.

### 👔 Manager

- **Login:** Manager can log in using predefined credentials.
- **Activate Customer Account:** Manager can activate a deactivated customer account.
- **Deactivate Customer Account:** Manager can deactivate an active customer account.
- **Assign Loan Applications:** Manager can assign customer loan applications to specific employees for processing.
- **Review Customer Feedback:** Manager can view all customer feedback and reset the feedback file after review.

### 👷 Employee

- **Login:** Employees can log in using their credentials to view assigned tasks.
- **Handle Loan Applications:** Employees can view loan applications assigned to them, process them (approve/reject), and update the customer's balance if approved.
- **Customer Handling:** Employees can view customer details, add new customers, remove customers, and view transaction histories.

### 👤 Customer

- **Login:** Customers can log in using their credentials to perform various banking operations.
- **View Account Balance:** Customers can view their current account balance.
- **Deposit Money:** Customers can deposit money into their accounts.
- **Withdraw Money:** Customers can withdraw money from their accounts.
- **Transfer Funds:** Customers can transfer money to other customers.
- **Apply for a Loan:** Customers can apply for loans by providing details such as loan amount, purpose, monthly income, and contact information.
- **Change Password:** Customers can change their account password.
- **Provide Feedback:** Customers can submit feedback about their banking experience.
- **View Transaction History:** Customers can view their past transactions.

---

## 📂 System Structure

```bash
BankingManagementSystem/
├── Admin/
│   ├── admin.c
│   └── admin.h
├── Manager/
│   ├── manager.c
│   └── manager.h
├── Employee/
│   ├── employee.c
│   └── employee.h
├── Customer/
│   ├── customer.c
│   ├── customer.h
│   ├── feedback.txt
│   ├── loans.txt
│   ├── transaction_history.txt
│   └── customers.txt
├── client.c
├── server.c
├── session.c
├── session.h
├── loan.h
├── manage_loan.txt
├── processed_loans_approved.txt
├── processed_loans_rejected.txt
└── README.md
```

### Setup

- **Compile the server**
  gcc server.c Admin/admin.c Customer/customer.c Employee/employee.c Manager/manager.c Session/session.c -o server -lpthread

- **Compile the client**
  gcc client.c Admin/admin.c Customer/customer.c Employee/employee.c Manager/manager.c Session/session.c -o client -lpthread

- **Run the server**
  ./server

- **Run the client**
  ./client
