CXX         := c++
CXXFLAGS    := -Wall -Wextra -Werror -std=c++98 -I./includes

# Directories
SRC_DIR     := src
OBJ_DIR     := obj

# Files
SRCS        := $(wildcard $(SRC_DIR)/*.cpp)
OBJS        := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
NAME        := webserv

TOTAL       := $(words $(SRCS))
COUNT       := 0

# Rules
all: $(NAME)

$(NAME): $(OBJS)
	@echo "\n-------------------- Building $(NAME) --------------------"
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"
	@echo "-------------------- We Still Use Wildcard Please Change!!!!!!!!! --------------------"

# Compile .cpp -> obj/.o with progress
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(eval COUNT=$(shell echo $$(($(COUNT)+1))))
	@printf "\rCompiling: [%-50s] %d/%d" \
	"$$(printf '#%.0s' $$(seq 1 $(COUNT)))" $(COUNT) $(TOTAL)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Tester – compiles every source except main.cpp, then links tester.cpp
TESTER_NAME := tester
TESTER_SRCS := $(filter-out $(SRC_DIR)/main.cpp, $(wildcard $(SRC_DIR)/*.cpp))
TESTER_OBJS := $(TESTER_SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

tester: $(TESTER_OBJS)
	@echo "\n-------------------- Building $(TESTER_NAME) --------------------"
	$(CXX) $(CXXFLAGS) $(TESTER_OBJS) -o $(TESTER_NAME)
	@echo "-------------------- Done --------------------"

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TESTER_NAME)

re: fclean all

.PHONY: all clean fclean re tester