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
	int stmtNum = 0;
	// iterate subsequent statements for variable/constant
	for (size_t i = 2; i < tokens.size(); i++) {
		string tempToken = tokens.at(i);
		cout << tempToken << endl;
		
		if (((tokens.at(i - 1) == "{") || (tokens.at(i - 1) == ";" || (tokens.at(i - 1) == "}")))
				&& (tempToken != "}") && (tempToken != "else")) {
			stmtNum++;
			Database::insertStmt(to_string(stmtNum));
			if (tokens.at(i + 1) == "=") {
				Database::insertAssignment(to_string(stmtNum));
				}
		}
		
		if (tempToken != "}") {
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
			else if (isdigit(tempToken[0]) && isdigit(tempToken[0])) {
				Database::insertConstant(tempToken);
			}
		}
	}

}		