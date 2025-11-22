/*
SPEC-1-Mess Timetable and Canteen Menu - Menu Driven C Program

This C program is an implementable, menu-driven console application based on the uploaded
synopsis for "Mess Timetable and Canteen Menu/Food Availability Checker".

Features implemented (MVP):
 - Login as Administrator or Student
 - Administrator: secure (obfuscated) password, update mess timetable, update canteen items,
   update availability, view complaints
 - Student: view mess timetable, view canteen menu & availability, submit complaints
 - Persistent storage using binary files: mess.dat, canteen.dat, complaints.dat, admin.dat
 - Simple password obfuscation for storage (illustrative; replace with real hashing for production)

Assumptions:
 - This is a console C program intended for learning / small deployment. For production use,
   replace the simple obfuscation with a secure salted hash (bcrypt/argon2) and use a proper
   authenticated backend and secure transport.
 - Limits: up to 100 canteen items, up to 2000 complaints, fixed 7x3 mess timetable.

Files used:
 - mess.dat
 - canteen.dat
 - complaints.dat
 - admin.dat

Submitted By: Students (from uploaded synopsis). See the uploaded synopsis for original requirements.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ITEMS 100
#define MAX_COMPLAINTS 2000
#define MAX_DESC 256
#define ADMIN_FILE "admin.dat"
#define CANTEEN_FILE "canteen.dat"
#define MESS_FILE "mess.dat"
#define COMPLAINTS_FILE "complaints.dat"

// Mess Timetable constants
#define DAYS 7
#define MEALS 3

// Simple obfuscation key (for demonstration only)
const unsigned char OBFUSCATION_KEY = 0xAA;

typedef struct {
    char name[64];
    float price;
    bool available;
    int id; // unique id
} CanteenItem;

typedef struct {
    int day; // 0..6
    int meal; // 0..2
    char dish[128];
} MessCell;

typedef struct {
    int id;
    char studentName[64];
    char description[MAX_DESC];
    char category[32];
    bool resolved;
} Complaint;

typedef struct {
    char username[64];
    unsigned char obfuscated_password[128]; // obfuscated stored password
} AdminRecord;

// In-memory stores
CanteenItem canteen[MAX_ITEMS];
int canteen_count = 0;
MessCell mess[DAYS][MEALS];
Complaint complaints[MAX_COMPLAINTS];
int complaints_count = 0;

// Utility: obfuscate / deobfuscate (XOR with constant) -> not secure, illustrative only
void obfuscate(const char *in, unsigned char *out) {
    size_t n = strlen(in);
    for (size_t i = 0; i < n; ++i) {
        out[i] = in[i] ^ OBFUSCATION_KEY;
    }
    out[n] = '\0' ^ OBFUSCATION_KEY; // store terminator
}

void deobfuscate(const unsigned char *in, char *out) {
    size_t i = 0;
    while (in[i] != (unsigned char)(('\0') ^ OBFUSCATION_KEY)) {
        out[i] = in[i] ^ OBFUSCATION_KEY;
        ++i;
    }
    out[i] = '\0';
}

// Persistence functions
void load_admin_or_init() {
    FILE *f = fopen(ADMIN_FILE, "rb");
    if (!f) {
        // create default admin (username: admin, password: admin123)
        AdminRecord a;
        strcpy(a.username, "admin");
        obfuscate("admin123", a.obfuscated_password);
        f = fopen(ADMIN_FILE, "wb");
        if (f) {
            fwrite(&a, sizeof(AdminRecord), 1, f);
            fclose(f);
        }
        return;
    }
    fclose(f);
}

bool admin_login() {
    char username[64];
    char password[64];
    printf("Enter admin username: ");
    scanf("%63s", username);
    printf("Enter admin password: ");
    scanf("%63s", password);

    AdminRecord a;
    FILE *f = fopen(ADMIN_FILE, "rb");
    if (!f) {
        printf("Admin file missing.\n");
        return false;
    }
    fread(&a, sizeof(AdminRecord), 1, f);
    fclose(f);

    if (strcmp(a.username, username) != 0) return false;
    char dec[128];
    deobfuscate(a.obfuscated_password, dec);
    if (strcmp(dec, password) == 0) return true;
    return false;
}

void save_canteen() {
    FILE *f = fopen(CANTEEN_FILE, "wb");
    if (!f) return;
    fwrite(&canteen_count, sizeof(int), 1, f);
    fwrite(canteen, sizeof(CanteenItem), canteen_count, f);
    fclose(f);
}

void load_canteen() {
    FILE *f = fopen(CANTEEN_FILE, "rb");
    if (!f) return;
    fread(&canteen_count, sizeof(int), 1, f);
    if (canteen_count > MAX_ITEMS) canteen_count = 0;
    fread(canteen, sizeof(CanteenItem), canteen_count, f);
    fclose(f);
}

void save_mess() {
    FILE *f = fopen(MESS_FILE, "wb");
    if (!f) return;
    fwrite(mess, sizeof(MessCell), DAYS*MEALS, f);
    fclose(f);
}

void load_mess() {
    FILE *f = fopen(MESS_FILE, "rb");
    if (!f) {
        // initialize with placeholders
        const char *defaults[DAYS][MEALS] = {
            {"Monday-Breakfast","Monday-Lunch","Monday-Dinner"},
            {"Tuesday-Breakfast","Tuesday-Lunch","Tuesday-Dinner"},
            {"Wednesday-Breakfast","Wednesday-Lunch","Wednesday-Dinner"},
            {"Thursday-Breakfast","Thursday-Lunch","Thursday-Dinner"},
            {"Friday-Breakfast","Friday-Lunch","Friday-Dinner"},
            {"Saturday-Breakfast","Saturday-Lunch","Saturday-Dinner"},
            {"Sunday-Breakfast","Sunday-Lunch","Sunday-Dinner"}
        };
        for (int d = 0; d < DAYS; ++d) {
            for (int m = 0; m < MEALS; ++m) {
                mess[d][m].day = d;
                mess[d][m].meal = m;
                strncpy(mess[d][m].dish, defaults[d][m], sizeof(mess[d][m].dish)-1);
                mess[d][m].dish[sizeof(mess[d][m].dish)-1] = '\0';
            }
        }
        save_mess();
        return;
    }
    fread(mess, sizeof(MessCell), DAYS*MEALS, f);
    fclose(f);
}

void save_complaints() {
    FILE *f = fopen(COMPLAINTS_FILE, "wb");
    if (!f) return;
    fwrite(&complaints_count, sizeof(int), 1, f);
    fwrite(complaints, sizeof(Complaint), complaints_count, f);
    fclose(f);
}

void load_complaints() {
    FILE *f = fopen(COMPLAINTS_FILE, "rb");
    if (!f) return;
    fread(&complaints_count, sizeof(int), 1, f);
    if (complaints_count > MAX_COMPLAINTS) complaints_count = 0;
    fread(complaints, sizeof(Complaint), complaints_count, f);
    fclose(f);
}

// Helper display functions
const char* day_name(int d) {
    static const char *names[DAYS] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
    return names[d % DAYS];
}
const char* meal_name(int m) {
    static const char *names[MEALS] = {"Breakfast","Lunch","Dinner"};
    return names[m % MEALS];
}

void view_mess_timetable() {
    printf("\n--- Mess Timetable ---\n");
    for (int d = 0; d < DAYS; ++d) {
        printf("%s:\n", day_name(d));
        for (int m = 0; m < MEALS; ++m) {
            printf("  %s: %s\n", meal_name(m), mess[d][m].dish);
        }
    }
    printf("----------------------\n");
}

void view_canteen_menu() {
    printf("\n--- Canteen Menu ---\n");
    for (int i = 0; i < canteen_count; ++i) {
        printf("%d) %s - Rs %.2f - %s\n", canteen[i].id, canteen[i].name, canteen[i].price,
               canteen[i].available ? "Available" : "Not Available");
    }
    if (canteen_count == 0) printf("No items available.\n");
    printf("--------------------\n");
}

// Admin operations
void admin_update_mess_timetable() {
    view_mess_timetable();
    int d, m;
    printf("Enter day index (0-Mon .. 6-Sun): ");
    if (scanf("%d", &d) != 1) return;
    printf("Enter meal index (0-Breakfast,1-Lunch,2-Dinner): ");
    if (scanf("%d", &m) != 1) return;
    getchar(); // consume newline
    if (d < 0 || d >= DAYS || m < 0 || m >= MEALS) { printf("Invalid indices.\n"); return; }
    printf("Enter new dish name: ");
    char buf[128];
    fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\n")] = '\0';
    strncpy(mess[d][m].dish, buf, sizeof(mess[d][m].dish)-1);
    mess[d][m].dish[sizeof(mess[d][m].dish)-1] = '\0';
    save_mess();
    printf("Timetable updated.\n");
}

void admin_add_canteen_item() {
    if (canteen_count >= MAX_ITEMS) { printf("Canteen full.\n"); return; }
    CanteenItem it;
    it.id = (canteen_count == 0) ? 1 : (canteen[canteen_count-1].id + 1);
    printf("Enter item name: ");
    getchar();
    fgets(it.name, sizeof(it.name), stdin);
    it.name[strcspn(it.name, "\n")] = '\0';
    printf("Enter price: ");
    scanf("%f", &it.price);
    it.available = true;
    canteen[canteen_count++] = it;
    save_canteen();
    printf("Item added.\n");
}

void admin_update_item_availability() {
    view_canteen_menu();
    int id;
    printf("Enter item id to toggle availability: ");
    if (scanf("%d", &id) != 1) return;
    for (int i = 0; i < canteen_count; ++i) {
        if (canteen[i].id == id) {
            canteen[i].available = !canteen[i].available;
            save_canteen();
            printf("Item availability toggled.\n");
            return;
        }
    }
    printf("Item id not found.\n");
}

void admin_view_complaints() {
    printf("\n--- Complaints ---\n");
    for (int i = 0; i < complaints_count; ++i) {
        printf("%d) %s | %s | %s | %s\n", complaints[i].id, complaints[i].studentName,
               complaints[i].category, complaints[i].description,
               complaints[i].resolved ? "Resolved" : "Pending");
    }
    if (complaints_count == 0) printf("No complaints.\n");
    printf("------------------\n");
}

// Student operations
void student_submit_complaint() {
    if (complaints_count >= MAX_COMPLAINTS) { printf("Complaint storage full.\n"); return; }
    Complaint c;
    c.id = (complaints_count == 0) ? 1 : (complaints[complaints_count-1].id + 1);
    getchar();
    printf("Enter your name: ");
    fgets(c.studentName, sizeof(c.studentName), stdin);
    c.studentName[strcspn(c.studentName, "\n")] = '\0';
    printf("Enter category (Mess/Canteen/Other): ");
    fgets(c.category, sizeof(c.category), stdin);
    c.category[strcspn(c.category, "\n")] = '\0';
    printf("Describe your complaint: \n");
    fgets(c.description, sizeof(c.description), stdin);
    c.description[strcspn(c.description, "\n")] = '\0';
    c.resolved = false;
    complaints[complaints_count++] = c;
    save_complaints();
    printf("Complaint submitted.\n");
}

// Main menus
void admin_menu() {
    if (!admin_login()) { printf("Login failed.\n"); return; }
    int choice;
    while (1) {
        printf("\n--- Admin Menu ---\n");
        printf("1) Update Mess Timetable\n");
        printf("2) Add Canteen Item\n");
        printf("3) Toggle Item Availability\n");
        printf("4) View Complaints\n");
        printf("5) Logout\n");
        printf("Choose: ");
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        switch (choice) {
            case 1: admin_update_mess_timetable(); break;
            case 2: admin_add_canteen_item(); break;
            case 3: admin_update_item_availability(); break;
            case 4: admin_view_complaints(); break;
            case 5: return;
            default: printf("Invalid choice.\n");
        }
    }
}

void student_menu() {
    int choice;
    while (1) {
        printf("\n--- Student Menu ---\n");
        printf("1) View Mess Timetable\n");
        printf("2) View Canteen Menu\n");
        printf("3) Submit Complaint\n");
        printf("4) Back\n");
        printf("Choose: ");
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        switch (choice) {
            case 1: view_mess_timetable(); break;
            case 2: view_canteen_menu(); break;
            case 3: student_submit_complaint(); break;
            case 4: return;
            default: printf("Invalid choice.\n");
        }
    }
}

int main() {
    // load data or create defaults
    load_admin_or_init();
    load_canteen();
    load_mess();
    load_complaints();

    int choice;
    while (1) {
        printf("\n===== Campus Dining App (Console) =====\n");
        printf("1) Login as Administrator\n");
        printf("2) Continue as Student\n");
        printf("3) Exit\n");
        printf("Choose: ");
        if (scanf("%d", &choice) != 1) { getchar(); continue; }
        switch (choice) {
            case 1: admin_menu(); break;
            case 2: student_menu(); break;
            case 3: printf("Exiting...\n"); exit(0);
            default: printf("Invalid option.\n");
        }
    }
    return 0;
}
