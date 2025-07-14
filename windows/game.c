#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <time.h>

char *server = "localhost";
char *user = "student";
char *password_db = "pass";
char *database = "last";


char currentUser[100];  


int tallyScore(char *name, char *mode, int win) {
    MYSQL *conn;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[3];

    conn = mysql_init(NULL);
    if (conn == NULL || mysql_real_connect(conn, server, user, password_db, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    // Step 1: Check if record exists
    stmt = mysql_stmt_init(conn);
    const char *checkQuery = "SELECT COUNT(*) FROM scoreboard WHERE name = ? AND mode = ?";
    if (!stmt || mysql_stmt_prepare(stmt, checkQuery, strlen(checkQuery))) {
        fprintf(stderr, "Check query failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    memset(bind, 0, sizeof(bind));

    // Bind name and mode
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = name;
    bind[0].buffer_length = strlen(name);

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = mode;
    bind[1].buffer_length = strlen(mode);

    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "Bind failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "Execution failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    // Bind result
    int count = 0;
    MYSQL_BIND resultBind[1];
    memset(resultBind, 0, sizeof(resultBind));
    resultBind[0].buffer_type = MYSQL_TYPE_LONG;
    resultBind[0].buffer = (char *)&count;

    mysql_stmt_bind_result(stmt, resultBind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    // Step 2: Insert or Update
    if (count == 0) {
        // INSERT
        stmt = mysql_stmt_init(conn);
        const char *insertQuery = "INSERT INTO scoreboard (name, mode, totalGamesPlayed, win) VALUES (?, ?, 1, ?)";
        mysql_stmt_prepare(stmt, insertQuery, strlen(insertQuery));

        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = name;
        bind[0].buffer_length = strlen(name);

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = mode;
        bind[1].buffer_length = strlen(mode);

        bind[2].buffer_type = MYSQL_TYPE_LONG;
        bind[2].buffer = (char *)&win;

        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
        mysql_stmt_close(stmt);
        printf("🆕 New record inserted!\n");
    } else {
        // UPDATE
        stmt = mysql_stmt_init(conn);
        const char *updateQuery = "UPDATE scoreboard SET totalGamesPlayed = totalGamesPlayed + 1, win = win + ? WHERE name = ? AND mode = ?";
        mysql_stmt_prepare(stmt, updateQuery, strlen(updateQuery));

        memset(bind, 0, sizeof(bind));
        int winIncrement = (win == 1) ? 1 : 0;

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = (char *)&winIncrement;

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = name;
        bind[1].buffer_length = strlen(name);

        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = mode;
        bind[2].buffer_length = strlen(mode);

        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);
        mysql_stmt_close(stmt);
        printf("🔁 Existing record updated!\n");
    }

    mysql_close(conn);
    return 1;
}




int login(char *username, char *password) {
    MYSQL *conn;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    MYSQL_RES *result_meta;
    unsigned int column_count;

    int isAuthenticated = 0;  // Flag to indicate if the user is authenticated

  
    // Initialize
    
    conn = mysql_init(NULL);
    if (conn == NULL || 
        mysql_real_connect(conn, server, user, password_db, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        return 0;
    }

    // Prepare statement
    stmt = mysql_stmt_init(conn);
    const char *query = "SELECT * FROM users WHERE email = ? AND password = ?";
    if (!stmt || mysql_stmt_prepare(stmt, query, strlen(query))) {
        fprintf(stderr, "Statement preparation failed: %s\n", mysql_error(conn));
        return 0;
    }

    // Bind input parameters
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = username;
    bind[0].buffer_length = strlen(username);

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = password;
    bind[1].buffer_length = strlen(password);

    if (mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr, "Parameter binding failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    // Execute statement
    if (mysql_stmt_execute(stmt)) {
        fprintf(stderr, "Query execution failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    // Get metadata to determine column count
    result_meta = mysql_stmt_result_metadata(stmt);
    if (!result_meta) {
        fprintf(stderr, "Failed to retrieve result metadata: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    column_count = mysql_num_fields(result_meta);

    // Allocate and bind result buffers
    MYSQL_BIND *result_bind = calloc(column_count, sizeof(MYSQL_BIND));
    char **buffers = calloc(column_count, sizeof(char *));
    unsigned long *lengths = calloc(column_count, sizeof(unsigned long));
    my_bool *is_null = calloc(column_count, sizeof(my_bool));
    my_bool *errors = calloc(column_count, sizeof(my_bool));

    for (unsigned int i = 0; i < column_count; ++i) {
        buffers[i] = calloc(100, sizeof(char));  // Adjust size as needed
        result_bind[i].buffer_type = MYSQL_TYPE_STRING;
        result_bind[i].buffer = buffers[i];
        result_bind[i].buffer_length = 100;
        result_bind[i].length = &lengths[i];
        result_bind[i].is_null = &is_null[i];
        result_bind[i].error = &errors[i];
    }

    if (mysql_stmt_bind_result(stmt, result_bind)) {
        fprintf(stderr, "Result bind failed: %s\n", mysql_error(conn));
        return EXIT_FAILURE;
    }

    // Fetch result
    int fetch_status = mysql_stmt_fetch(stmt);
    if (fetch_status == 0) {
        isAuthenticated = 1;  // Set authentication flag  
      strcpy(currentUser, buffers[3]);  // Copy the value before freeing

       
    }else{
         isAuthenticated = 0;
    }
  
    // Clean up
    mysql_free_result(result_meta);
    mysql_stmt_close(stmt);
    mysql_close(conn);

    for (unsigned int i = 0; i < column_count; ++i) {
        free(buffers[i]);
    }
    free(buffers);
    free(result_bind);
    free(lengths);
    free(is_null);
    free(errors);

    return isAuthenticated;
}


void precise_sleep_ms(long milliseconds) {
    LARGE_INTEGER freq, start, current;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    long long elapsed;
    do {
        QueryPerformanceCounter(&current);
        elapsed = (current.QuadPart - start.QuadPart) * 1000 / freq.QuadPart;
    } while (elapsed < milliseconds);
}



struct modes
{
    int speed;
    int count; 
    int min;
    int max; 

};


int speed; 
int count; 
int min; 
int max;


 
char *currentMode;

struct modes easy = {1000, 5, 1, 5};
struct modes medium = {750, 10, 1, 20};
struct modes hard = {500, 15, 1, 30};

          




void difficulties() {

    while (1)
    {
         system("cls");
         printf("Difficulties:\n");
         printf("1. Easy   ( Speed: %d ms, Count: %d, Range: [%d, %d]\n", easy.speed, easy.count, easy.min, easy.max);
         printf("2. Medium ( Speed: %d ms, Count: %d, Range: [%d, %d]\n", medium.speed, medium.count, medium.min, medium.max);
         printf("3. Hard   ( Speed: %d ms, Count: %d, Range: [%d, %d]\n", hard.speed, hard.count, hard.min, hard.max);
         printf("4. Custom\n");
         printf("0. Back to menu\n");

        int choice;
        scanf("%d", &choice);
        if (choice == 0) {
            break; // Exit the loop
        } else if (choice == 1) {
    printf("You selected Easy mode.\n");
    speed = easy.speed;
    count = easy.count; 
    min = easy.min;
    max = easy.max;
    currentMode = "easy";  // 🧩 Add this!
}else if (choice == 2) {
            printf("You selected Medium mode.\n");
              speed = medium.speed;
            count = medium.count; 
            min = medium.min;
            max = medium.max;
           currentMode = "medium";
        } else if (choice == 3) {
            printf("You selected Hard mode.\n");
                 speed = hard.speed;
            count = hard.count; 
            min = hard.min;
            max = hard.max;
           currentMode = "hard";
        } else if (choice == 4) {
            printf("You selected Custom mode.\n");
            printf("Please enter speed in ms: ");
            scanf("%d", &speed);
            printf("Please enter count: ");
            scanf("%d", &count);
            printf("Please enter min value: ");
            scanf("%d", &min);
            printf("Please enter max value: ");
            scanf("%d", &max);
            currentMode = "custom";  // 🧩 Add this!
        } else {
            printf("Invalid choice, please try again.\n");
        }

        
    }

         printf(" Current Settings   ( Speed: %d ms, Count: %d, Range: [%d, %d]\n", speed, count, min, max);

         }

void play(char *username) {
     int realAnswer = 0 ;
     int userAnswer ;

    srand(time(NULL));
    for (int i = 0; i < count; i++) {
        
        int randomNumber = (rand() % (max - min + 1)) + min;
        system("cls");
        // Create command string
        char command[200];
        snprintf(command, sizeof(command), "figlet    '.              '%d'               .' | boxes -d bear", randomNumber);

        // Run the command
        system(command);

        realAnswer += randomNumber;
      
        fflush(stdout);
        precise_sleep_ms(speed);
    }
    system("cls");
   
    system("figlet 'Enter Total : ' | boxes -d tux");
    printf("\n : " );
    scanf("%d", &userAnswer);

    if(userAnswer == realAnswer) printf("Nays ka\n");
    else printf("Real Answer was : %d\n", realAnswer);
    // int tallyScore(char *name, char *mode, int totalGamesPlayed, int win) 
    tallyScore( username, currentMode,  userAnswer == realAnswer ? 1 : 0);
    printf("\nPress Enter to continue...");
    getchar(); // Consume the newline character left by scanf
    getchar(); // Wait for the user to press Enter
    system("cls");
}


void menu(char *username) {
    
    while (1) {
        system("cls");
        printf("Welcome, %s!\n", username);
        printf("Current Settings   ( Speed: %d ms, Count: %d, Range: [%d, %d]\n", speed, count, min, max);
        printf("Current Difficulty : %s\n", currentMode);
        printf("Menu:\n");
        printf("1. Play \n");   
        printf("2. Difficuties \n");
        printf("0. Exit\n");

        int choice;
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 0) {
            break; // Exit the loop
        } else if (choice == 1) {
            play(username);
        } else if (choice == 2) {
            difficulties();
        } else {
            printf("Invalid choice, please try again.\n");
        }
    }
  
}

void setToEasy(){
    speed = easy.speed;
    count = easy.count; 
    min = easy.min;
    max = easy.max;
    currentMode = "easy";  
}


void promptLogin() {
    char username[100];
    char password[100];

    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);

    setToEasy();


    int result = login(username, password);
    if (result) {
        printf("Login successful!\n");
        menu(currentUser);
    } else {
        printf("Login failed.\n");
    }

}
int user_exists(MYSQL *conn, const char *username) {
    char query[256];
    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM users WHERE email='%s'", username);

    if (mysql_query(conn, query)) {
        printf("User check failed: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        printf("Failed to store result: %s\n", mysql_error(conn));
        return -1;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    int exists = row && atoi(row[0]) > 0;
    mysql_free_result(res);
    return exists;
}

int register_user() {
    char username[50], password[50], name[50];
    printf("Register a new user\n");
    printf("Username: ");
    scanf("%49s", username);
    printf("Password: ");
    scanf("%49s", password);
    printf("Name: ");   
    scanf("%49s", name);

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL || 
        mysql_real_connect(conn, server, user, password_db, database, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Connection failed: %s\n", mysql_error(conn));
        return 0;
    }

    int exists = user_exists(conn, username);
    if (exists == -1) {
        mysql_close(conn);
        return 0;
    } else if (exists) {
        printf("Username '%s' already exists. Choose another.\n", username);
        mysql_close(conn);
        return 0;
    }

    char query[512];
    snprintf(query, sizeof(query),
        "INSERT INTO users(email, password, name) VALUES('%s', '%s','%s')",
        username, password, name);

    if (mysql_query(conn, query)) {
        printf("Registration failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }

    printf("User '%s' registered successfully!\n", username);
    mysql_close(conn);
    return 1;
}



int main() {
    
    printf("Welcome to the Game!\n");
    printf("1. Login\n");
    printf("2. Register\n");
    printf("0. Exit\n");

    int choice;
    printf("Enter your choice: ");
    scanf("%d", &choice);
    if (choice == 0) {
        printf("Exiting...\n");
        return 0;
    } else if (choice == 1) {
        promptLogin();
    } else if (choice == 2) {
        if (register_user()) {
            printf("Registration successful! You can now login.\n");
            system("cls");
            printf("Login with your new credentials.\n");
            promptLogin();
        } else {
            printf("Registration failed. Please try again.\n");
        }
    } else {
        printf("Invalid choice, please try again.\n");
    }

    return 0;
}