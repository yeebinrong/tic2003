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

	// iterate subsequent statements for variable/constant
	for (int i = 2; i < tokens.size(); i++) {
		//for (string token : tokens) {
		string tempToken = tokens.at(i);
		string tempAssignToken = "";
		if (tempToken != "}") {
			if ((tempToken[0] > 96) && ((tokens.at(i + 1) == "="))) {		// =
				Database::insertVariable(tempToken);
				int x = i - 2;
				while (tokens.at(x) != ";") {
					tempAssignToken += tokens.at(x);
					x++;
				}
				//Database::insertAssignment(tempAssignToken);
			}
			else if ((tempToken != "}") && (tokens.at(i - 1) == "read")) {
				Database::insertVariable(tempToken);
				//Database::insertAssignment(tokens.at(i - 1));
				//Database::insertAssignment(tempToken);
			}
		}
		else if ((tempToken >= "1") && (tempToken <= "9")) {	//constants
			Database::insertConstant(tempToken);
		}
		

	}

}