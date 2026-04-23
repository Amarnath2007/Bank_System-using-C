/*
 * ============================================================
 *        ADVANCED BANK MANAGEMENT SYSTEM
 *                 Developed in C
 * ============================================================
 * DEFAULT ADMIN CREDENTIALS:
 *   Username : admin
 *   Password : admin123
 *
 * SAMPLE FLOW:
 *   Main Menu -> [1] Admin -> Login -> Create Account
 *   Main Menu -> [2] User  -> Enter AccNo + Password -> ATM Menu
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
    #define CLEAR "cls"
#else
    #include <termios.h>
    #include <unistd.h>
    #define CLEAR "clear"
#endif

/* ─────────────────── CONSTANTS ─────────────────── */
#define ACCOUNTS_FILE     "accounts.dat"
#define TRANSACTIONS_FILE "transactions.dat"
#define MAX_NAME          50
#define MAX_PASSWORD      20
#define MAX_ACCOUNTS      1000
#define MAX_TRANSACTIONS  500
#define ADMIN_USER        "admin"
#define ADMIN_PASS        "admin123"
#define MAX_LOGIN_TRIES   3
#define BASE_ACC_NO       100001

/* ─────────────────── STRUCTURES ─────────────────── */
typedef struct {
    int    accNo;
    char   name[MAX_NAME];
    char   password[MAX_PASSWORD];
    double balance;
    int    active;   /* 1 = active, 0 = deleted */
} Account;

typedef struct {
    int    txnId;
    int    accNo;
    char   type[20];       /* Deposit / Withdraw / Transfer / Received */
    double amount;
    int    relatedAcc;     /* For transfers: destination/source acc no */
    char   timestamp[30];
} Transaction;

/* ─────────────────── PROTOTYPES ─────────────────── */
void   clearScreen(void);
void   printBanner(void);
void   printLine(char ch, int len);
void   printHeader(const char *title);
void   pressEnter(void);
void   getPassword(char *pwd, int maxLen);

int    loadAccounts(Account *arr, int *count);
int    saveAccounts(Account *arr, int count);
int    loadTransactions(Transaction *arr, int *count);
int    saveTransaction(Transaction *t);

int    generateAccNo(Account *arr, int count);
int    findAccount(Account *arr, int count, int accNo);
void   logTransaction(int accNo, const char *type, double amount, int relatedAcc);

int    getInt(const char *prompt, int min, int max);
double getDouble(const char *prompt, double min);
void   getString(const char *prompt, char *buf, int maxLen);

void   adminMenu(void);
void   adminLogin(void);
void   createAccount(void);
void   deleteAccount(void);
void   viewAllAccounts(void);
void   searchAccount(void);
void   sortByBalance(void);
void   viewUserTransactions(void);

void   userMenu(void);
void   userLogin(void);
void   atmMenu(Account *acc, Account *allAcc, int count);
void   checkBalance(Account *acc);
void   depositMoney(Account *acc, Account *allAcc, int count);
void   withdrawMoney(Account *acc, Account *allAcc, int count);
void   transferMoney(Account *acc, Account *allAcc, int count);
void   viewMyTransactions(Account *acc);
void   changePassword(Account *acc, Account *allAcc, int count);

/* ===================================================
 *                      MAIN
 * =================================================== */
int main(void) {
    int choice;
    clearScreen();
    printBanner();

    while (1) {
        printHeader("MAIN MENU");
        printf("  [1]  Admin Panel\n");
        printf("  [2]  User / ATM\n");
        printf("  [0]  Exit\n");
        printLine('-', 48);
        choice = getInt("Select", 0, 2);

        switch (choice) {
            case 1: adminMenu(); break;
            case 2: userMenu();  break;
            case 0:
                clearScreen();
                printLine('=', 45);
                printf("  Thank you for using Amar's Bank. Goodbye!\n");
                printLine('=', 45);
                printf("\n");
                exit(0);
        }
    }
    return 0;
}

/* ===================================================
 *                   UI HELPERS
 * =================================================== */
void clearScreen(void) {
    system(CLEAR);
}

void printBanner(void) {
    printf("      ADVANCED BANK MANAGEMENT SYSTEM\n");
    printf("             [ Amar's Bank v1.0 ]\n");
}

void printLine(char ch, int len) {
    int i;
    for (i = 0; i < len; i++) putchar(ch);
    putchar('\n');
}

void printHeader(const char *title) {
    printf("\n");
    printLine('=', 48);
    printf("                   %s\n", title);
    printLine('=', 48);
}

void pressEnter(void) {
    int c;
    printf("\n  Press ENTER to continue...");
    fflush(stdout);
    while ((c = getchar()) != '\n' && c != EOF);
    getchar();
}

/* Cross-platform masked password input */
void getPassword(char *pwd, int maxLen) {
#ifdef _WIN32
    int  i  = 0;
    char ch;
    while (i < maxLen - 1) {
        ch = (char)_getch();
        if (ch == '\r' || ch == '\n') break;
        if (ch == '\b') {
            if (i > 0) { i--; printf("\b \b"); }
        } else {
            pwd[i++] = ch;
            printf("*");
        }
    }
    pwd[i] = '\0';
    printf("\n");
#else
    struct termios oldt, newt;
    int  i  = 0;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    while (i < maxLen - 1 && (ch = (char)getchar()) != '\n' && ch != EOF) {
        if (ch == '\b' || ch == 127) {
            if (i > 0) { i--; printf("\b \b"); fflush(stdout); }
        } else {
            pwd[i++] = ch;
            printf("*");
            fflush(stdout);
        }
    }
    pwd[i] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
#endif
}

/* ===================================================
 *                FILE OPERATIONS
 * =================================================== */
int loadAccounts(Account *arr, int *count) {
    FILE *fp = fopen(ACCOUNTS_FILE, "rb");
    if (!fp) { *count = 0; return 0; }
    *count = 0;
    while (fread(&arr[*count], sizeof(Account), 1, fp) == 1)
        (*count)++;
    fclose(fp);
    return 1;
}

int saveAccounts(Account *arr, int count) {
    FILE *fp = fopen(ACCOUNTS_FILE, "wb");
    if (!fp) return 0;
    fwrite(arr, sizeof(Account), count, fp);
    fclose(fp);
    return 1;
}

int loadTransactions(Transaction *arr, int *count) {
    FILE *fp = fopen(TRANSACTIONS_FILE, "rb");
    if (!fp) { *count = 0; return 0; }
    *count = 0;
    while (fread(&arr[*count], sizeof(Transaction), 1, fp) == 1)
        (*count)++;
    fclose(fp);
    return 1;
}

int saveTransaction(Transaction *t) {
    FILE *fp = fopen(TRANSACTIONS_FILE, "ab");
    if (!fp) return 0;
    fwrite(t, sizeof(Transaction), 1, fp);
    fclose(fp);
    return 1;
}

/* ===================================================
 *              ACCOUNT UTILITIES
 * =================================================== */
int generateAccNo(Account *arr, int count) {
    int i, maxNo;
    if (count == 0) return BASE_ACC_NO;
    maxNo = 0;
    for (i = 0; i < count; i++)
        if (arr[i].accNo > maxNo) maxNo = arr[i].accNo;
    return maxNo + 1;
}

int findAccount(Account *arr, int count, int accNo) {
    int i;
    for (i = 0; i < count; i++)
        if (arr[i].accNo == accNo && arr[i].active == 1)
            return i;
    return -1;
}

void logTransaction(int accNo, const char *type, double amount, int relatedAcc) {
    Transaction txns[MAX_TRANSACTIONS];
    Transaction t;
    time_t      now;
    struct tm  *tm_info;
    int         count = 0;

    loadTransactions(txns, &count);

    t.txnId      = count + 1;
    t.accNo      = accNo;
    t.amount     = amount;
    t.relatedAcc = relatedAcc;
    strncpy(t.type, type, sizeof(t.type) - 1);
    t.type[sizeof(t.type) - 1] = '\0';

    now     = time(NULL);
    tm_info = localtime(&now);
    strftime(t.timestamp, sizeof(t.timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    saveTransaction(&t);
}

/* ===================================================
 *               INPUT VALIDATION
 * =================================================== */
int getInt(const char *prompt, int min, int max) {
    int  val;
    char buf[50];
    while (1) {
        printf("\n  %s (%d-%d): ", prompt, min, max);
        if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        printf("  [!] Invalid. Enter a number between %d and %d.\n", min, max);
    }
}

double getDouble(const char *prompt, double min) {
    double val;
    char   buf[50];
    while (1) {
        printf("\n  %s (min %.2f): ", prompt, min);
        if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
        if (sscanf(buf, "%lf", &val) == 1 && val >= min)
            return val;
        printf("  [!] Invalid. Enter a number >= %.2f\n", min);
    }
}

void getString(const char *prompt, char *buf, int maxLen) {
    char tmp[256];
    while (1) {
        printf("  %s: ", prompt);
        if (fgets(tmp, sizeof(tmp), stdin) == NULL) continue;
        tmp[strcspn(tmp, "\n")] = '\0';
        if (strlen(tmp) > 0 && (int)strlen(tmp) < maxLen) {
            strncpy(buf, tmp, maxLen - 1);
            buf[maxLen - 1] = '\0';
            return;
        }
        printf("  [!] Input must be 1-%d characters.\n", maxLen - 1);
    }
}

/* ===================================================
 *                  ADMIN MODULE
 * =================================================== */
void adminMenu(void) {
    clearScreen();
    adminLogin();
}

void adminLogin(void) {
    char user[30];
    char pass[30];
    int  tries = 0;
    int  ch;

    printHeader("ADMIN LOGIN");

    while (tries < MAX_LOGIN_TRIES) {
        printf("\n  Username: ");
        fflush(stdout);
        if (fgets(user, sizeof(user), stdin) == NULL) return;
        user[strcspn(user, "\n")] = '\0';

        printf("  Password: ");
        fflush(stdout);
        getPassword(pass, sizeof(pass));

        if (strcmp(user, ADMIN_USER) == 0 && strcmp(pass, ADMIN_PASS) == 0) {
            printf("\n  [OK] Login successful! Welcome, Admin.\n");
            pressEnter();

            /* Admin control panel loop */
            while (1) {
                clearScreen();
                printHeader("ADMIN CONTROL PANEL");
                printf("  [1]  Create New Account\n");
                printf("  [2]  Delete Account\n");
                printf("  [3]  View All Accounts\n");
                printf("  [4]  Search Account\n");
                printf("  [5]  Sort Accounts by Balance\n");
                printf("  [6]  View User Transaction History\n");
                printf("  [0]  Logout\n");
                printLine('-', 48);
                ch = getInt("Select", 0, 6);

                switch (ch) {
                    case 1: createAccount();        break;
                    case 2: deleteAccount();        break;
                    case 3: viewAllAccounts();      break;
                    case 4: searchAccount();        break;
                    case 5: sortByBalance();        break;
                    case 6: viewUserTransactions(); break;
                    case 0:
                        printf("\n  [OK] Admin logged out.\n");
                        pressEnter();
                        return;
                }
            }
        } else {
            tries++;
            printf("\n  [X] Invalid credentials. Attempts left: %d\n",
                   MAX_LOGIN_TRIES - tries);
        }
    }
    printf("\n  [!] Too many failed attempts. Access locked.\n");
    pressEnter();
}

void createAccount(void) {
    Account accounts[MAX_ACCOUNTS];
    Account newAcc;
    int     count   = 0;
    double  initDep = 0.0;

    loadAccounts(accounts, &count);
    clearScreen();
    printHeader("CREATE NEW ACCOUNT");

    memset(&newAcc, 0, sizeof(newAcc));
    newAcc.accNo  = generateAccNo(accounts, count);
    newAcc.active = 1;

    getString("Full Name", newAcc.name, MAX_NAME);

    printf("  Set Password: ");
    fflush(stdout);
    getPassword(newAcc.password, MAX_PASSWORD);

    if ((int)strlen(newAcc.password) < 4) {
        printf("  [!] Password too short (min 4 chars). Account not created.\n");
        pressEnter();
        return;
    }

    initDep        = getDouble("Initial Deposit (min Rs.500)", 500.0);
    newAcc.balance = initDep;

    accounts[count] = newAcc;
    count++;

    if (saveAccounts(accounts, count)) {
        logTransaction(newAcc.accNo, "Deposit", initDep, 0);
        printLine('-', 48);
        printf("  [OK] Account created successfully!\n");
        printf("  +----------------------------------+\n");
        printf("  | Account Number : %-13d |\n", newAcc.accNo);
        printf("  | Account Holder : %-13s |\n", newAcc.name);
        printf("  | Initial Balance: Rs.%-10.2f |\n", newAcc.balance);
        printf("  +----------------------------------+\n");
        printf("  [!] Save your account number!\n");
    } else {
        printf("  [X] Error saving account.\n");
    }
    pressEnter();
}

void deleteAccount(void) {
    Account accounts[MAX_ACCOUNTS];
    int     count   = 0;
    int     accNo   = 0;
    int     idx     = -1;
    int     confirm = 0;

    loadAccounts(accounts, &count);
    clearScreen();
    printHeader("DELETE ACCOUNT");

    accNo = getInt("Enter Account Number", 100001, 999999);
    idx   = findAccount(accounts, count, accNo);

    if (idx == -1) {
        printf("\n  [X] Account not found or already deleted.\n");
        pressEnter();
        return;
    }

    printf("\n  Account Holder : %s\n",   accounts[idx].name);
    printf("  Balance        : Rs.%.2f\n", accounts[idx].balance);
    confirm = getInt("Confirm delete? (1=Yes / 0=No)", 0, 1);

    if (confirm == 1) {
        accounts[idx].active = 0;
        if (saveAccounts(accounts, count))
            printf("\n  [OK] Account %d deleted successfully.\n", accNo);
        else
            printf("\n  [X] Error deleting account.\n");
    } else {
        printf("\n  [i] Delete cancelled.\n");
    }
    pressEnter();
}

void viewAllAccounts(void) {
    Account accounts[MAX_ACCOUNTS];
    int     count = 0;
    int     i, found;

    loadAccounts(accounts, &count);
    clearScreen();
    printHeader("ALL ACTIVE ACCOUNTS");

    found = 0;
    printf("  %-10s %-22s %-15s\n", "Acc No.", "Name", "Balance (Rs.)");
    printLine('-', 52);

    for (i = 0; i < count; i++) {
        if (accounts[i].active) {
            printf("  %-10d %-22s Rs.%-12.2f\n",
                   accounts[i].accNo,
                   accounts[i].name,
                   accounts[i].balance);
            found++;
        }
    }

    if (!found) printf("\n  [i] No active accounts found.\n");
    else        printf("\n  Total Active Accounts: %d\n", found);

    pressEnter();
}

void searchAccount(void) {
    Account  accounts[MAX_ACCOUNTS];
    Account *a;
    int      count = 0;
    int      accNo = 0;
    int      idx   = -1;

    loadAccounts(accounts, &count);
    clearScreen();
    printHeader("SEARCH ACCOUNT");

    accNo = getInt("Enter Account Number", 100001, 999999);
    idx   = findAccount(accounts, count, accNo);

    if (idx == -1) {
        printf("\n  [X] Account not found.\n");
    } else {
        a = &accounts[idx];
        printLine('-', 48);
        printf("  Account Number : %d\n",      a->accNo);
        printf("  Account Holder : %s\n",      a->name);
        printf("  Balance        : Rs.%.2f\n", a->balance);
        printf("  Status         : Active\n");
        printLine('-', 48);
    }
    pressEnter();
}

void sortByBalance(void) {
    Account accounts[MAX_ACCOUNTS];
    Account active[MAX_ACCOUNTS];
    Account tmp;
    int     count  = 0;
    int     aCount = 0;
    int     i, j;

    loadAccounts(accounts, &count);
    clearScreen();
    printHeader("ACCOUNTS SORTED BY BALANCE (High to Low)");

    for (i = 0; i < count; i++) {
        if (accounts[i].active) {
            active[aCount] = accounts[i];
            aCount++;
        }
    }

    /* Bubble sort descending */
    for (i = 0; i < aCount - 1; i++) {
        for (j = 0; j < aCount - i - 1; j++) {
            if (active[j].balance < active[j+1].balance) {
                tmp          = active[j];
                active[j]    = active[j+1];
                active[j+1]  = tmp;
            }
        }
    }

    printf("  %-6s %-10s %-22s %-15s\n", "Rank", "Acc No.", "Name", "Balance (Rs.)");
    printLine('-', 58);
    for (i = 0; i < aCount; i++) {
        printf("  #%-5d %-10d %-22s Rs.%-12.2f\n",
               i+1, active[i].accNo, active[i].name, active[i].balance);
    }

    if (!aCount) printf("\n  [i] No active accounts.\n");

    pressEnter();
}

void viewUserTransactions(void) {
    Account     accounts[MAX_ACCOUNTS];
    Transaction txns[MAX_TRANSACTIONS];
    int         aCount = 0;
    int         tCount = 0;
    int         accNo  = 0;
    int         found  = 0;
    int         i;

    loadAccounts(accounts, &aCount);
    clearScreen();
    printHeader("VIEW USER TRANSACTIONS");

    accNo = getInt("Enter Account Number", 100001, 999999);
    if (findAccount(accounts, aCount, accNo) == -1) {
        printf("\n  [X] Account not found.\n");
        pressEnter();
        return;
    }

    loadTransactions(txns, &tCount);
    printf("\n  Transactions for Account #%d\n", accNo);
    printLine('-', 68);
    printf("  %-5s %-12s %-14s %-10s %-20s\n",
           "ID", "Type", "Amount(Rs.)", "Related", "Timestamp");
    printLine('-', 68);

    for (i = 0; i < tCount; i++) {
        if (txns[i].accNo == accNo) {
            printf("  %-5d %-12s Rs.%-11.2f %-10d %s\n",
                   txns[i].txnId,
                   txns[i].type,
                   txns[i].amount,
                   txns[i].relatedAcc,
                   txns[i].timestamp);
            found++;
        }
    }
    if (!found) printf("  [i] No transactions found.\n");

    pressEnter();
}

/* ===================================================
 *               USER / ATM MODULE
 * =================================================== */
void userMenu(void) {
    clearScreen();
    userLogin();
}

void userLogin(void) {
    Account accounts[MAX_ACCOUNTS];
    char    buf[50];
    char    pass[MAX_PASSWORD];
    int     count = 0;
    int     accNo = 0;
    int     idx   = -1;
    int     tries = 0;

    loadAccounts(accounts, &count);
    printHeader("USER LOGIN");

    printf("\n  Enter Account Number: ");
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL) return;
    if (sscanf(buf, "%d", &accNo) != 1) {
        printf("  [X] Invalid account number.\n");
        pressEnter();
        return;
    }

    idx = findAccount(accounts, count, accNo);
    if (idx == -1) {
        printf("\n  [X] Account not found.\n");
        pressEnter();
        return;
    }

    while (tries < MAX_LOGIN_TRIES) {
        printf("  Password: ");
        fflush(stdout);
        getPassword(pass, sizeof(pass));

        if (strcmp(pass, accounts[idx].password) == 0) {
            printf("\n  [OK] Login successful! Welcome, %s.\n", accounts[idx].name);
            pressEnter();
            atmMenu(&accounts[idx], accounts, count);
            return;
        } else {
            tries++;
            printf("\n  [X] Wrong password. Attempts left: %d\n",
                   MAX_LOGIN_TRIES - tries);
        }
    }
    printf("\n  [!] Account locked after %d failed attempts.\n", MAX_LOGIN_TRIES);
    pressEnter();
}

void atmMenu(Account *acc, Account *allAcc, int count) {
    int ch;
    while (1) {
        clearScreen();
        printHeader("ATM - USER PANEL");
        printf("  Welcome, %s  |  Acc No: %d\n", acc->name, acc->accNo);
        printLine('-', 48);
        printf("  [1]  Check Balance\n");
        printf("  [2]  Deposit Money\n");
        printf("  [3]  Withdraw Money\n");
        printf("  [4]  Transfer Money\n");
        printf("  [5]  Transaction History\n");
        printf("  [6]  Change Password\n");
        printf("  [0]  Logout\n");
        printLine('-', 48);
        ch = getInt("Select", 0, 6);

        switch (ch) {
            case 1: checkBalance(acc);                  break;
            case 2: depositMoney(acc, allAcc, count);   break;
            case 3: withdrawMoney(acc, allAcc, count);  break;
            case 4: transferMoney(acc, allAcc, count);  break;
            case 5: viewMyTransactions(acc);            break;
            case 6: changePassword(acc, allAcc, count); break;
            case 0:
                printf("\n  [OK] Logged out. Goodbye, %s!\n", acc->name);
                pressEnter();
                return;
        }
    }
}

void checkBalance(Account *acc) {
    clearScreen();
    printHeader("ACCOUNT BALANCE");
    printLine('-', 48);
    printf("  Account Number : %d\n",      acc->accNo);
    printf("  Account Holder : %s\n",      acc->name);
    printf("  Available Bal  : Rs.%.2f\n", acc->balance);
    printLine('-', 48);
    pressEnter();
}

void depositMoney(Account *acc, Account *allAcc, int count) {
    double amount;
    int    i;

    clearScreen();
    printHeader("DEPOSIT MONEY");

    amount = getDouble("Enter Deposit Amount (min Rs.1)", 1.0);
    acc->balance += amount;

    for (i = 0; i < count; i++) {
        if (allAcc[i].accNo == acc->accNo) {
            allAcc[i].balance = acc->balance;
            break;
        }
    }

    if (saveAccounts(allAcc, count)) {
        logTransaction(acc->accNo, "Deposit", amount, 0);
        printf("\n  [OK] Rs.%.2f deposited successfully!\n", amount);
        printf("  Updated Balance: Rs.%.2f\n", acc->balance);
    } else {
        printf("\n  [X] Transaction failed.\n");
        acc->balance -= amount;
    }
    pressEnter();
}

void withdrawMoney(Account *acc, Account *allAcc, int count) {
    double amount;
    int    i;

    clearScreen();
    printHeader("WITHDRAW MONEY");

    printf("  Current Balance: Rs.%.2f\n", acc->balance);
    amount = getDouble("Enter Withdrawal Amount (min Rs.1)", 1.0);

    if (amount > acc->balance) {
        printf("\n  [X] Insufficient balance! Available: Rs.%.2f\n", acc->balance);
        pressEnter();
        return;
    }

    acc->balance -= amount;

    for (i = 0; i < count; i++) {
        if (allAcc[i].accNo == acc->accNo) {
            allAcc[i].balance = acc->balance;
            break;
        }
    }

    if (saveAccounts(allAcc, count)) {
        logTransaction(acc->accNo, "Withdraw", amount, 0);
        printf("\n  [OK] Rs.%.2f withdrawn successfully!\n", amount);
        printf("  Remaining Balance: Rs.%.2f\n", acc->balance);
    } else {
        printf("\n  [X] Transaction failed.\n");
        acc->balance += amount;
    }
    pressEnter();
}

void transferMoney(Account *acc, Account *allAcc, int count) {
    double amount;
    int    destAccNo;
    int    destIdx;
    int    i;

    clearScreen();
    printHeader("TRANSFER MONEY");

    printf("  Your Balance: Rs.%.2f\n", acc->balance);

    destAccNo = getInt("Enter Destination Account Number", 100001, 999999);
    if (destAccNo == acc->accNo) {
        printf("\n  [X] Cannot transfer to your own account.\n");
        pressEnter();
        return;
    }

    destIdx = findAccount(allAcc, count, destAccNo);
    if (destIdx == -1) {
        printf("\n  [X] Destination account not found.\n");
        pressEnter();
        return;
    }

    printf("  Destination : %s (Acc #%d)\n", allAcc[destIdx].name, destAccNo);

    amount = getDouble("Enter Transfer Amount (min Rs.1)", 1.0);
    if (amount > acc->balance) {
        printf("\n  [X] Insufficient balance!\n");
        pressEnter();
        return;
    }

    acc->balance            -= amount;
    allAcc[destIdx].balance += amount;

    for (i = 0; i < count; i++) {
        if (allAcc[i].accNo == acc->accNo)
            allAcc[i].balance = acc->balance;
    }

    if (saveAccounts(allAcc, count)) {
        logTransaction(acc->accNo, "Transfer", amount, destAccNo);
        logTransaction(destAccNo,  "Received", amount, acc->accNo);
        printf("\n  [OK] Rs.%.2f transferred to %s (Acc #%d)!\n",
               amount, allAcc[destIdx].name, destAccNo);
        printf("  Remaining Balance: Rs.%.2f\n", acc->balance);
    } else {
        printf("\n  [X] Transfer failed. Rolling back.\n");
        acc->balance            += amount;
        allAcc[destIdx].balance -= amount;
        for (i = 0; i < count; i++) {
            if (allAcc[i].accNo == acc->accNo)
                allAcc[i].balance = acc->balance;
        }
    }
    pressEnter();
}

void viewMyTransactions(Account *acc) {
    Transaction txns[MAX_TRANSACTIONS];
    int         tCount = 0;
    int         found  = 0;
    int         i;
    char        rel[15];

    clearScreen();
    printHeader("MY TRANSACTION HISTORY");
    loadTransactions(txns, &tCount);

    printf("  %-5s %-12s %-14s %-10s %-20s\n",
           "ID", "Type", "Amount(Rs.)", "Related", "Timestamp");
    printLine('-', 65);

    for (i = 0; i < tCount; i++) {
        if (txns[i].accNo == acc->accNo) {
            if (txns[i].relatedAcc == 0)
                strcpy(rel, "---");
            else
                sprintf(rel, "#%d", txns[i].relatedAcc);

            printf("  %-5d %-12s Rs.%-11.2f %-10s %s\n",
                   txns[i].txnId,
                   txns[i].type,
                   txns[i].amount,
                   rel,
                   txns[i].timestamp);
            found++;
        }
    }
    if (!found) printf("  [i] No transactions yet.\n");

    pressEnter();
}

void changePassword(Account *acc, Account *allAcc, int count) {
    char oldPass[MAX_PASSWORD];
    char newPass[MAX_PASSWORD];
    char confirm[MAX_PASSWORD];
    int  i;

    clearScreen();
    printHeader("CHANGE PASSWORD");

    printf("  Current Password: ");
    fflush(stdout);
    getPassword(oldPass, sizeof(oldPass));

    if (strcmp(oldPass, acc->password) != 0) {
        printf("\n  [X] Incorrect current password.\n");
        pressEnter();
        return;
    }

    printf("  New Password: ");
    fflush(stdout);
    getPassword(newPass, sizeof(newPass));

    if ((int)strlen(newPass) < 4) {
        printf("\n  [!] Password too short (minimum 4 characters).\n");
        pressEnter();
        return;
    }

    printf("  Confirm New Password: ");
    fflush(stdout);
    getPassword(confirm, sizeof(confirm));

    if (strcmp(newPass, confirm) != 0) {
        printf("\n  [X] Passwords do not match.\n");
        pressEnter();
        return;
    }

    strncpy(acc->password, newPass, MAX_PASSWORD - 1);
    acc->password[MAX_PASSWORD - 1] = '\0';

    for (i = 0; i < count; i++) {
        if (allAcc[i].accNo == acc->accNo) {
            strncpy(allAcc[i].password, newPass, MAX_PASSWORD - 1);
            allAcc[i].password[MAX_PASSWORD - 1] = '\0';
            break;
        }
    }

    if (saveAccounts(allAcc, count))
        printf("\n  [OK] Password changed successfully!\n");
    else
        printf("\n  [X] Failed to save new password.\n");

    pressEnter();
}

/*
 * ─────────────────────────────────────────────────
 *  HOW TO USE IN DEV-C++
 * ─────────────────────────────────────────────────
 *  1. Open Dev-C++
 *  2. File -> New -> Source File
 *  3. Paste this entire code
 *  4. File -> Save As -> main.c
 *  5. Execute -> Compile & Run  (F11)
 *
 *  ADMIN : Username = admin | Password = admin123
 *  USER  : Use account number + password set at creation
 *
 *  DATA  : accounts.dat & transactions.dat are auto-
 *          created in the same folder as the .exe
 * ─────────────────────────────────────────────────
 */
