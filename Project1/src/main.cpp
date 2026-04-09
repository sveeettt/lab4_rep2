#include <iostream>
#include <string>
#include <limits>
#include "Database.h"
#include "Auth.h"
#include "Queries.h"
#include "Functions.h"

using namespace std;

void showMenu(bool isAdmin) {
    cout << "\n========== МУЗЫКАЛЬНЫЙ САЛОН ==========" << endl;
    cout << "1. Показать информацию о проданных и оставшихся компактах" << endl;
    cout << "2. Показать продажи компакта за период" << endl;
    cout << "3. Показать самый популярный компакт" << endl;
    cout << "4. Показать самого популярного исполнителя" << endl;
    cout << "5. Показать статистику по авторам" << endl;
    cout << "6. Заполнить статистику за период (функция)" << endl;
    cout << "7. Показать продажи компакта за период (функция)" << endl;

    if (isAdmin) {
        cout << "\n--- АДМИНИСТРАТИВНЫЕ ФУНКЦИИ ---" << endl;
        cout << "8. Добавить новый компакт-диск" << endl;
        cout << "9. Добавить операцию (поступление/продажа)" << endl;
        cout << "10. Обновить информацию о компакте" << endl;
        cout << "11. Удалить компакт-диск" << endl;
    }

    cout << "0. Выход" << endl;
    cout << "===================================" << endl;
    cout << "Выберите действие: ";
}

void addNewCD(sqlite3* db) {
    int disc_id;
    string date, manufacturer;
    double price;

    cout << "\n--- ДОБАВЛЕНИЕ НОВОГО КОМПАКТ-ДИСКА ---" << endl;
    cout << "Введите ID диска: ";
    cin >> disc_id;
    cin.ignore();
    cout << "Введите дату изготовления (ГГГГ-ММ-ДД): ";
    getline(cin, date);
    cout << "Введите производителя: ";
    getline(cin, manufacturer);
    cout << "Введите цену: ";
    cin >> price;

    string sql = "INSERT INTO cd_disc (disc_id, manufacture_date, manufacturer, price) VALUES (" +
        to_string(disc_id) + ", '" + date + "', '" + manufacturer + "', " + to_string(price) + ");";

    if (executeSQL(db, sql)) {
        cout << "Компакт-диск успешно добавлен!" << endl;
    }
}

void addTransaction(sqlite3* db) {
    int disc_id, quantity;
    string date, type;

    cout << "\n--- ADD OPERATION ---" << endl;
    cout << "Enter disc ID: ";
    cin >> disc_id;
    cin.ignore();
    cout << "Enter date (YYYY-MM-DD): ";
    getline(cin, date);
    cout << "Enter operation type (income/sale): ";
    getline(cin, type);
    cout << "Enter quantity: ";
    cin >> quantity;

    string sql = "INSERT INTO \"TRANSACTION\" (operation_date, operation_type, disc_id, quantity) VALUES ('" +
        date + "', '" + type + "', " + to_string(disc_id) + ", " + to_string(quantity) + ");";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cout << "SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
    }
    else {
        cout << "Operation added!" << endl;
        // Обновляем stock
        Database dbObj("");
        dbObj.updateSTOCKFromTRANSACTIONs();
    }
}

int main() {
    string db_path;
    cout << "Введите путь к файлу базы данных: ";
    cin >> db_path;

    Database db(db_path);
    if (!db.connect()) {
        cerr << "Не удалось подключиться к базе данных!" << endl;
        return 1;
    }

    // Создаем триггер для предотвращения перепродажи
    createPreventOverSaleTrigger(db.getDB());

    string username, password;
    cout << "\n=== АУТЕНТИФИКАЦИЯ ===" << endl;
    cout << "Логин: ";
    cin >> username;
    cout << "Пароль: ";
    cin >> password;

    User currentUser;
    if (!authenticate(db.getDB(), username, password, currentUser)) {
        cout << "Ошибка аутентификации! Неверный логин или пароль." << endl;
        return 1;
    }

    cout << "\nДобро пожаловать, " << currentUser.username << "!" << endl;
    cout << "Ваша роль: " << currentUser.role << endl;

    bool isAdmin = (currentUser.role == "admin");
    int choice;

    do {
        showMenu(isAdmin);
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1:
            showSoldAndRemaining(db.getDB());
            break;

        case 2: {
            int disc_id;
            string start, end;
            cout << "Введите ID компакт-диска: ";
            cin >> disc_id;
            cout << "Введите начальную дату (ГГГГ-ММ-ДД): ";
            cin >> start;
            cout << "Введите конечную дату (ГГГГ-ММ-ДД): ";
            cin >> end;
            showSalesByDiscAndPeriod(db.getDB(), disc_id, start, end);
            break;
        }

        case 3:
            showMostSoldDiscDetails(db.getDB());
            break;

        case 4:
            showTopPerformerSales(db.getDB());
            break;

        case 5:
            showAuthorStats(db.getDB());
            break;

        case 6: {
            string start, end;
            cout << "Введите начальную дату (ГГГГ-ММ-ДД): ";
            cin >> start;
            cout << "Введите конечную дату (ГГГГ-ММ-ДД): ";
            cin >> end;
            fillPeriodStats(db.getDB(), start, end);
            break;
        }

        case 7: {
            int disc_id;
            string start, end;
            cout << "Введите ID компакт-диска: ";
            cin >> disc_id;
            cout << "Введите начальную дату (ГГГГ-ММ-ДД): ";
            cin >> start;
            cout << "Введите конечную дату (ГГГГ-ММ-ДД): ";
            cin >> end;
            showDiscSalesForPeriod(db.getDB(), disc_id, start, end);
            break;
        }

        case 8:
            if (isAdmin) addNewCD(db.getDB());
            else cout << "Доступ запрещен!" << endl;
            break;

        case 9:
            if (isAdmin) addTransaction(db.getDB());
            else cout << "Доступ запрещен!" << endl;
            break;

        case 10: {
            if (isAdmin) {
                int disc_id;
                double new_price;
                cout << "Введите ID диска для обновления цены: ";
                cin >> disc_id;
                cout << "Введите новую цену: ";
                cin >> new_price;
                string sql = "UPDATE CD_DISC SET price = " + to_string(new_price) + " WHERE disc_id = " + to_string(disc_id) + ";";
                if (executeSQL(db.getDB(), sql)) {
                    cout << "Цена обновлена!" << endl;
                }
            }
            else {
                cout << "Доступ запрещен!" << endl;
            }
            break;
        }

        case 11:
            if (isAdmin) {
                int disc_id;
                cout << "Введите ID диска для удаления: ";
                cin >> disc_id;
                string sql = "DELETE FROM CD_DISC WHERE disc_id = " + to_string(disc_id) + ";";
                if (executeSQL(db.getDB(), sql)) {
                    cout << "Компакт-диск удален!" << endl;
                    Database dbObj("");
                    dbObj.updateSTOCKFromTRANSACTIONs();
                }
            }
            else {
                cout << "Доступ запрещен!" << endl;
            }
            break;

        case 0:
            cout << "До свидания!" << endl;
            break;

        default:
            cout << "Неверный выбор!" << endl;
        }

    } while (choice != 0);

    return 0;
}