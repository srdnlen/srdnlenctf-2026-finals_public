#include <iostream>
#include <string>
#include <memory>
#include <array>
#include <limits>

#define AMMO_QUANTITY 10
#define BUDGET 1000
#define HANDGUN_PRICE 33
#define SHOTGUN_PRICE 50
#define RIFLE_PRICE 51

class LongGunAmmo {
public:
    virtual void fire() = 0;
    virtual ~LongGunAmmo() = default;
};

class HandgunAmmo {
public:
    std::array<char, 0x210> description;

    void fire() {
        std::cout << "[SIDEARM] Loading compact handgun rounds..." << std::endl;
        std::cout << "[SIDEARM] Ballistic profile: " << description.data() << std::endl;
        std::cout << "[SIDEARM] Close-range test completed." << std::endl;
    }
};

class ShotgunShellAmmo : public LongGunAmmo {
public:
    std::array<char, 0x420> description;

    void fire() override {
        std::cout << "[SMOOTHBORE] Loading 12-gauge shotgun shells..." << std::endl;
        std::cout << "[SMOOTHBORE] Ballistic profile: " << description.data() << std::endl;
        std::cout << "[SMOOTHBORE] Wide pellet spread across the range." << std::endl;
    }
};

class RifleRoundAmmo : public LongGunAmmo {
public:
    std::array<char, 0x630> description;

    void fire() override {
        std::cout << "[RIFLED] Loading long-range rifle rounds..." << std::endl;
        std::cout << "[RIFLED] Ballistic profile: " << description.data() << std::endl;
        std::cout << "[RIFLED] Stabilized spin trajectory confirmed." << std::endl;
    }
};

std::array<std::unique_ptr<HandgunAmmo>, AMMO_QUANTITY> handgun_crate;
std::array<std::unique_ptr<ShotgunShellAmmo>, AMMO_QUANTITY> shotgun_crate;
std::array<std::unique_ptr<RifleRoundAmmo>, AMMO_QUANTITY> rifle_crate;

static const char ammo_choice[] = "[1] Handgun Ammo\n[2] Shotgun Shell Ammo\n[3] Rifle Round Ammo";
static const char no_ammo_on_crate[] = "No ammo of that type on the crate...";
static const char invalid_choice[] = "Invalid choice.";
static const std::string ammo_choice_prices =
    "[1] Handgun Ammo " + std::to_string(HANDGUN_PRICE) + "$\n" +
    "[2] Shotgun Shell Ammo " + std::to_string(SHOTGUN_PRICE) + "$\n" +
    "[3] Rifle Round Ammo " + std::to_string(RIFLE_PRICE) + "$";
static const char banner[] =
    "Welcome to the Ammunition Factory!\n"
    "It's not a hard job, but there is a lot of work to do!\n"
    "Also...today we have a very limited budget...";
static const char menu_strings[] = 
    "[1] Craft new ammo\n"
    "[2] Discard ammo\n"
    "[3] Fire ammo\n"
    "[4] Describe ammo\n"
    "[5] Quit";

bool check_budget(int& budget, int cost){
    if (budget < 0) {
        std::cout << "You made us go broke! Go find a new job!" << std::endl;
        exit(0);
    }
    if (budget < cost) {
        std::cout << "You don't have enough money..." << std::endl;
        return false;
    }
    return true;
}

int get_valid_serial_number(int quantity) {
    int index;

    std::cout << "Insert the Serial Number:" << std::endl;
    std::cout << ">> ";

    if (!(std::cin >> index)) {
        std::cout << "Invalid Serial Number" << std::endl;
        exit(-1);
    }
    if (index < 0 || index >= quantity) {
        std::cout << "Invalid Serial Number" << std::endl;
        exit(-1);
    }

    return index;
}

int readChoice(){
    int choice{};
    if (!(std::cin >> choice)) {
        if (std::cin.eof())
            exit(0);
        std::cin.clear();
        std::cin.ignore(
            std::numeric_limits<std::streamsize>::max(), '\n'
        );
        return -1;
    }
    return choice;
}

void craft_ammo(int& budget){
    int index;
    std::cout << "Which ammo type do you want to craft now?" << std::endl;
    std::cout << ammo_choice_prices << std::endl;
    std::cout << ">> ";
    int choice = readChoice();
    switch(choice){
        case 1:
            if (!check_budget(budget, HANDGUN_PRICE)) {
                return;
            }
            budget -= HANDGUN_PRICE;
            index = get_valid_serial_number(AMMO_QUANTITY);
            handgun_crate[index] = std::unique_ptr<HandgunAmmo>(new HandgunAmmo);
            break;
        case 2:
            if (!check_budget(budget, SHOTGUN_PRICE)) {
                return;
            }
            budget -= SHOTGUN_PRICE;
            index = get_valid_serial_number(AMMO_QUANTITY);
            shotgun_crate[index] = std::unique_ptr<ShotgunShellAmmo>(new ShotgunShellAmmo);
            break;
        case 3:
            if (!check_budget(budget, RIFLE_PRICE)) {
                return;
            }
            budget -= RIFLE_PRICE;
            index = get_valid_serial_number(AMMO_QUANTITY);
            rifle_crate[index] = std::unique_ptr<RifleRoundAmmo>(new RifleRoundAmmo);
            break;
        default:
            std::cout << invalid_choice << std::endl;
            break;
    }
}

void discard_ammo(int fired, int index=0){
    int choice;
    if (!fired){
        std::cout << "Which defective ammo do you want to discard?" << std::endl;
        std::cout << ammo_choice << std::endl;
        std::cout << ">> ";
        choice = readChoice();
    }else{
        choice = fired;
    }
    switch(choice){
        case 1:
            if (!fired) {
                index = get_valid_serial_number(AMMO_QUANTITY);
            }
            if (handgun_crate[index] == nullptr) {
                std::cout << no_ammo_on_crate << std::endl;
                return;
            }
            handgun_crate[index].reset();
            break;
        case 2:
            if (!fired) {
                index = get_valid_serial_number(AMMO_QUANTITY);
            }
            if (shotgun_crate[index] == nullptr) {
                std::cout << no_ammo_on_crate << std::endl;
                return;
            }
            shotgun_crate[index].reset();
            break;
        case 3:
            if (!fired) {
                index = get_valid_serial_number(AMMO_QUANTITY);
            }
            if (rifle_crate[index] == nullptr) {
                std::cout << no_ammo_on_crate << std::endl;
                return;
            }
            rifle_crate[index].reset();
            break;
        default:
            std::cout << invalid_choice << std::endl;
            break;
    }
}

void fire_ammo(){
    int index;
    std::cout << "Which ammo do you want to fire?" << std::endl;
    std::cout << ammo_choice << std::endl;
    std::cout << ">> ";
    int choice = readChoice();
    switch(choice){
        case 1:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (handgun_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            handgun_crate[index]->fire();
            discard_ammo(choice, index);
            break;
        case 2:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (shotgun_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            shotgun_crate[index]->fire();
            discard_ammo(choice, index);
            break;
        case 3:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (rifle_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            rifle_crate[index]->fire();
            discard_ammo(choice, index);
            break;
        default:
            std::cout << invalid_choice << std::endl;
            break;
    }
}

void add_description(int index, int choice){
    auto read_description = [](auto *obj){
        if (obj == nullptr) {
            std::cout << no_ammo_on_crate << std::endl;
            return;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Insert the description of the ammo:" << std::endl;
        std::cout << ">> ";
        std::cin.getline(obj->description.data(), obj->description.size() + 1);
    };

    switch(choice){
        case 1:
            read_description(handgun_crate[index].get());
            break;
        case 2:
            read_description(shotgun_crate[index].get());
            break;
        case 3:
            read_description(rifle_crate[index].get());
            break;
        default:
            std::cout << invalid_choice << std::endl;
            return;
    }
}

void describe_ammo(){
    int index;
    std::cout << "Which ammo do you want to describe?" << std::endl;
    std::cout << ammo_choice << std::endl;
    std::cout << ">> ";
    int choice = readChoice();
    switch(choice){
        case 1:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (handgun_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            add_description(index, choice);
            break;
        case 2:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (shotgun_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            add_description(index, choice);
            break;
        case 3:
            index = get_valid_serial_number(AMMO_QUANTITY);
            if (rifle_crate[index] == nullptr){
                std::cout << no_ammo_on_crate << std::endl;
                break;
            }
            add_description(index, choice);
            break;
        default:
            std::cout << invalid_choice << std::endl;
            break;
    }
}

void win(){
    system("/bin/sh");
    exit(0);
}

int menu(){
    std::cout << menu_strings << std::endl;
    std::cout << ">> ";
    return readChoice();
}

int main(){
    int budget = BUDGET;
    std::cout << banner << std::endl;
    while(true){
        std::cout << "Your budget is: " << budget << " $" << std::endl;
        check_budget(budget, 0);
        std::cout << "What do you want to do?" << std::endl;
        switch(menu()){
            case 1:
                craft_ammo(budget);
                break;
            case 2:
                discard_ammo(0);
                break;
            case 3:
                fire_ammo();
                break;
            case 4:
                describe_ammo();
                break;
            case 5:
                std::cout << "Quitting..." << std::endl;
                exit(0);
                break;
            default:
                std::cout << invalid_choice << std::endl;
                break;
        }
    } 
    return 0;
}