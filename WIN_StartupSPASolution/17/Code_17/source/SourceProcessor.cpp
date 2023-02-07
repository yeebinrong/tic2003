#include "SourceProcessor.h"
#include <iostream>

// method for processing the source program
// This method currently only inserts the procedure name into the database
// using some highly simplified logic.
// You should modify this method to complete the logic for handling all the required syntax.
void SourceProcessor::process(string program) {
	// initialize the database
	Database::initialize();

	// tokenize the program
	Tokenizer tk;
	vector<string> tokens;
	tk.tokenize(program, tokens);

	// This logic is highly simplified based on iteration 1 requirements and 
	// the assumption that the programs are valid.
	string procedureName = tokens.at(1);
	// insert the procedure into the database
	Database::insertProcedure(procedureName);

	//@@@ init @@@//
	int stmtNum = 0; //statement increment
	int prevStmtNum = 0; //statement-1 + skip 0->1
	int containerStmtNum = 0; //reference statement number for container (if/while)
	string prevState = "main"; //main, if, while
	string curState = "main"; //main, if, else, while

	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {
		string tempToken = tokens.at(i);

		
		if ((((tokens.at(i - 1) == "{") || (tokens.at(i - 1) == ";") || (tokens.at(i - 1) == "}")))
				&& (tempToken != "}") && (tempToken != "else")) {	//start of statement

				prevStmtNum = stmtNum;
				stmtNum++;
				Database::insertStmt(to_string(stmtNum));
				
				//Handle Next Insert
				if (prevStmtNum && curState != "else" ) {
					Database::insertNext(to_string(prevStmtNum), to_string(stmtNum), "1");
					//TODO - logic to delete rows in indirect next
				}
				else if (curState == "else") {
					prevState = "else";
					curState = "normal";
				}

				//Handle container (if/while)
				if (tempToken == "if" || tempToken == "while" ) {
					containerStmtNum = stmtNum;
					prevState = curState;
					curState = tempToken;
				}

				//Handle Assignment Insert
				if (tokens.at(i + 1) == "=") {
					Database::insertAssignment(to_string(stmtNum));
				}
		}
		else if (tempToken == "}") {
				if (curState == "while") {
					Database::insertNext(to_string(stmtNum), to_string(containerStmtNum), "1"); //while loop
					Database::insertNext(to_string(containerStmtNum), to_string(stmtNum+1), "1"); //out of while-loop
					curState = "normal";
				}
				else if (curState == "if") {
					containerStmtNum = stmtNum;
					curState = "else";
				}
				else if (prevState == "else") {
					prevState = "normal";
					Database::insertNext(to_string(containerStmtNum), to_string(stmtNum+1), "1");
				}
		}
		else if (tempToken != "}") {
				if (isalpha(tempToken[0]) && tokens.at(i + 1) == "=") {
					Database::insertVariable(tempToken);
				}
				else if (tokens.at(i - 1) == "read") {
					Database::insertVariable(tempToken);
					Database::insertRead(to_string(stmtNum));
				}
				else if (tokens.at(i - 1) == "print") {
					Database::insertVariable(tempToken);
					Database::insertPrint(to_string(stmtNum));
				}
				else if (isdigit(tempToken[0])) {
					Database::insertConstant(tempToken);
				}
		}
	}

	//Handle Indirect-Next insert
	//for (int i = 1; i < stmtNum; i++) {
	//	for (int x = i+1; x <= stmtNum; x++) {
	//		Database::insertNext(to_string(i), to_string(x), "0");
	//	}
	//}

}		