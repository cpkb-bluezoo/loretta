# Loretta Runtime Library Makefile
# Builds runtime classes into loretta.jar

JAVAC = javac
JAR = jar
JAVAC_FLAGS = -source 21 -target 21 -Xlint:-options

SRC_DIR = src
BUILD_DIR = classes
JAR_FILE = loretta.jar

.PHONY: all clean jar

all: $(JAR_FILE)
	@echo "Runtime library built: $(JAR_FILE)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# List of all source files for dependency tracking
JAVA_SOURCES = \
	$(SRC_DIR)/$$O.java $(SRC_DIR)/$$I.java $(SRC_DIR)/$$F.java \
	$(SRC_DIR)/$$S.java $(SRC_DIR)/$$N.java $(SRC_DIR)/$$B.java \
	$(SRC_DIR)/$$X.java $(SRC_DIR)/$$L.java $(SRC_DIR)/$$T.java \
	$(SRC_DIR)/$$D.java $(SRC_DIR)/$$MH.java $(SRC_DIR)/$$BS.java \
	$(SRC_DIR)/$$G.java $(SRC_DIR)/$$C.java $(SRC_DIR)/$$BY.java \
	$(SRC_DIR)/$$ST.java $(SRC_DIR)/$$FS.java $(SRC_DIR)/$$File.java \
	$(SRC_DIR)/$$SL.java $(SRC_DIR)/$$BM.java $(SRC_DIR)/$$GE.java \
	$(SRC_DIR)/$$Cls.java $(SRC_DIR)/$$Inst.java $(SRC_DIR)/$$Mod.java \
	$(SRC_DIR)/$$Gen.java $(SRC_DIR)/$$Future.java $(SRC_DIR)/$$Async.java \
	$(SRC_DIR)/$$Super.java $(SRC_DIR)/$$SM.java $(SRC_DIR)/$$CM.java \
	$(SRC_DIR)/$$Prop.java $(SRC_DIR)/$$MV.java

# Compile all Java files - use shell glob with quotes to handle $ in filenames
# Touch stamp file to track compilation time
.compile.stamp: $(JAVA_SOURCES) | $(BUILD_DIR)
	cd $(SRC_DIR) && $(JAVAC) $(JAVAC_FLAGS) -d ../$(BUILD_DIR) \
		'$$O.java' '$$I.java' '$$F.java' '$$S.java' '$$N.java' \
		'$$B.java' '$$X.java' '$$L.java' '$$T.java' '$$D.java' \
		'$$MH.java' '$$BS.java' '$$G.java' '$$C.java' '$$BY.java' \
		'$$ST.java' '$$FS.java' '$$File.java' '$$SL.java' '$$BM.java' \
		'$$GE.java' '$$Cls.java' '$$Inst.java' '$$Mod.java' '$$Gen.java' \
		'$$Future.java' '$$Async.java' '$$Super.java' '$$SM.java' \
		'$$CM.java' '$$Prop.java' '$$MV.java'
	@touch .compile.stamp

# Create the JAR file from compiled classes
$(JAR_FILE): .compile.stamp
	$(JAR) cf $(JAR_FILE) -C $(BUILD_DIR) .
	@echo "Created $(JAR_FILE)"

jar: $(JAR_FILE)

clean:
	rm -rf $(BUILD_DIR) $(JAR_FILE) .compile.stamp
