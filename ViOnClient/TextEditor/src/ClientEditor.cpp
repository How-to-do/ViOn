#include <thread>
#include <utility>
#include <ncurses.h>

#include "NCursesView.h"
#include "Parser.h"
#include "ClientEditor.h"
#include "FileController/ChangeApplier.h"

#include "FileController/JsonParser.h"

#define CREATE_FILE 1
#define CONNECT_TO_FILE 2
#define EXIT 'q'

int mainMenu();

void listenServ(Client& client, std::shared_ptr<FileStorage> file, View& view) {
    TextEditor textEditor(view);
    Change change;
    while (change.cmd != CLOSE_CONNECT) {
        change = client.recvChanges();

        ChangeApplier change_applier(change, file);
        change_applier.applyChange();


//        std::list <SymbolState> :: iterator it;
//
//        for (it = file->symbols.begin(); it != file->symbols.end(); it++) {
//            err << (*it).symbol << " ";
//        }

        err << "Change: " << JsonParser::ParseToJson(change) << std::endl;
        ParserForEditor parser(change, file);
        Text text(parser.parse());
        err << "listenServ: " << text.getTextStr() << std::endl;


        std::string str;

        clear();
        for (auto i : file->symbols) {
            if (i.symbol != '\0') {
                str.push_back(i.symbol);
            }
        }

        err << "Dats: " << str << std::endl;
        mvprintw(0, 0, "%s", str.c_str());
        refresh();
    }
}

ClientEditor::ClientEditor(const std::string& file_name, int port, std::string host) : port_(port), host_(std::move(host)), file_(file_name) {
    try {
        client_.connectToServer(host_, port_);
        initscr();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    changeCreator_.AddChangeCreator(new ChangeCreatorCreateFile);
    changeCreator_.AddChangeCreator(new ChangeCreatorDeleteFile);
    changeCreator_.AddChangeCreator(new ChangeCreatorDeleteString);
    changeCreator_.AddChangeCreator(new ChangeCreatorDeleteSymbol);
    changeCreator_.AddChangeCreator(new ChangeCreatorInsertSubString);
    changeCreator_.AddChangeCreator(new ChangeCreatorMoveCursor);

    file_storage_ = std::make_shared<FileStorage>();
}

void ClientEditor::startEdit() {
    while (true) {
        int cmd = mainMenu();
        switch (cmd) {
            case CREATE_FILE:
                createFileView();
                break;
            case CONNECT_TO_FILE:
                connectToFileView();
                break;
            case EXIT:
                endwin();
                return;
            default:
                break;
        }
    }
}

void ClientEditor::save() {
    for (SymbolState s : file_storage_->symbols) {
        if (s.is_visible) {
            file_ << s.symbol;
        }
    }
    file_ << std::endl;
}

void ClientEditor::edit() {
    Change change;
//    Position pos = {0};

    while (change.cmd != CLOSE_CONNECT) {
        char change_c;
        std::cin >> change_c;

        change.cmd = (change_c == '#') ? CLOSE_CONNECT : INSERT_SYMBOL;
        change.symbol = change_c;

        client_.sendChanges(change);
    }
    save();
}

int mainMenu() {
    std::vector<std::string_view> menu {
            "Menu",
            "F1 - Create file",
            "F2 - Connect to file",
            "Q  - Exit"
    };

    int row, col;
    getmaxyx(stdscr, row, col);

    clear();
    for (size_t i = 0; i < menu.size(); ++i) {
        mvwaddstr(stdscr, row / 2 - menu.size() / 2 + i, (col - menu.at(i).length()) / 2, menu.at(i).data());
    }
    mvprintw(0,0,"%d %d %d %d", KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT);
    refresh();
    curs_set(0);
    keypad(stdscr, true);
    noecho();
    while (true) {
        switch (getch()) {
            case KEY_F(1):
                clear();
                return CREATE_FILE;
            case KEY_F(2):
                clear();
                return CONNECT_TO_FILE;
            case EXIT:
                clear();
                return EXIT;
            default:
                mvwaddstr(stdscr, row - 1, 0, "Print F1 or F2");
                break;
        };
    }
}

void ClientEditor::createFileView() {
    int row = getmaxy(stdscr);
    int file_id = client_.createNewFile();
    if (file_id != -1) {
        mvprintw(row - 1, 0, "file_id = %d", file_id);
        file_storage_->file_id = file_id;
        move(0, 0);
        runTextEditor(file_id);
    } else {
        clear();
        mvprintw(row - 1, 0, "File creating failed. Press any key to get back to main menu...", file_id);
        getch();
    }
}

void ClientEditor::connectToFileView() {
    int row = getmaxy(stdscr);
    mvwaddstr(stdscr, row - 1, 0, "Input file id: ");
    curs_set(1);
    echo();
    size_t file_id = 0;
    scanw("%zd", &file_id);
    clear();
    mvprintw(row - 1, 0, "Connecting to file with id=%d. Please wait...", file_id);
    if (client_.connectToFile(file_id) != -1) {
        runTextEditor(file_id);
    } else {
        clear();
        mvprintw(row - 1, 0, "Connection failed. Press any key to get back to main menu...", file_id);
        getch();
    }
}

void ClientEditor::runTextEditor(int file_id) {
    Interpretator interpretator(changeCreator_);
    NCursesView view(interpretator, client_, file_id);

    std::thread thread_listen(listenServ, std::ref(client_), file_storage_, std::ref(view));
    thread_listen.detach();

    view.listen();

}

//void show(std::string_view text) {
//    clear();
//    mvprintw(0, 0, "%s", text.data());
//}
