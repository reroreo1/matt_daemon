NAME = MattDaemon

CXXFLAGS = -Wall -Wextra -Werror -std=c++17
INCLUDES = -I./include

SRC_DIR = src
OBJ_DIR = obj

SRC = $(SRC_DIR)/main.cpp \
	$(SRC_DIR)/Matt_daemon.cpp \
	$(SRC_DIR)/Tintin_reporter.cpp \
	$(SRC_DIR)/Server.cpp

OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

all: $(NAME)

$(NAME): $(OBJ_DIR) $(OBJ)
	g++ $(CXXFLAGS) $(OBJ) -o $(NAME)
	@echo "Compilation complete. Run with sudo ./$(NAME)"

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	g++ $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

run: all
	sudo ./$(NAME)

.PHONY: all clean fclean re run
