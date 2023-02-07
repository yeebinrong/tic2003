#include "Database.h"

sqlite3* Database::dbConnection;
vector<vector<string>> Database::dbResults;
char* Database::errorMessage;

// method to connect to the database and initialize tables in the database
void Database::initialize() {
	// open a database connection and store the pointer into dbConnection
	sqlite3_open("database.db", &dbConnection);

	// drop the existing procedure table (if any)
	string dropProcedureTableSQL = "DROP TABLE IF EXISTS procedures";
	sqlite3_exec(dbConnection, dropProcedureTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a procedure table
	string createProcedureTableSQL = "CREATE TABLE procedures ( name VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createProcedureTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing variable table (if any)
	string dropVariableTableSQL = "DROP TABLE IF EXISTS variables";
	sqlite3_exec(dbConnection, dropVariableTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a variable table
	string createVariableTableSQL = "CREATE TABLE variables ( name VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createVariableTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing constant table (if any)
	string dropConstantTableSQL = "DROP TABLE IF EXISTS constants";
	sqlite3_exec(dbConnection, dropConstantTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a constant table
	string createConstantTableSQL = "CREATE TABLE constants ( value VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createConstantTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing assign table (if any)
	string dropAssignTableSQL = "DROP TABLE IF EXISTS assigns";
	sqlite3_exec(dbConnection, dropAssignTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a assign table
	string createAssignTableSQL = "CREATE TABLE assigns ( stmtNo VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createAssignTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing print table (if any)
	string dropPrintTableSQL = "DROP TABLE IF EXISTS prints";
	sqlite3_exec(dbConnection, dropPrintTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a print table
	string createPrintTableSQL = "CREATE TABLE prints ( stmtNo VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createPrintTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing read table (if any)
	string dropReadTableSQL = "DROP TABLE IF EXISTS reads";
	sqlite3_exec(dbConnection, dropReadTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a read table
	string createReadTableSQL = "CREATE TABLE reads ( stmtNo VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createReadTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing stmt table (if any)
	string dropStmtTableSQL = "DROP TABLE IF EXISTS stmts";
	sqlite3_exec(dbConnection, dropStmtTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a stmt table
	string createStmtTableSQL = "CREATE TABLE stmts ( stmtNo VARCHAR(255) PRIMARY KEY);";
	sqlite3_exec(dbConnection, createStmtTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing next table (if any)
	string dropNextTableSQL = "DROP TABLE IF EXISTS nexts";
	sqlite3_exec(dbConnection, dropNextTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a next table
	string createNextTableSQL = "CREATE TABLE nexts ( stmtNo VARCHAR(255), nextStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, nextStmtNo));";
	sqlite3_exec(dbConnection, createNextTableSQL.c_str(), NULL, 0, &errorMessage);

	// drop the existing parent table (if any)
	string dropParentTableSQL = "DROP TABLE IF EXISTS parents";
	sqlite3_exec(dbConnection, dropParentTableSQL.c_str(), NULL, 0, &errorMessage);
	// create a parent table
	string createParentTableSQL = "CREATE TABLE parents ( stmtNo VARCHAR(255), childStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, childStmtNo));";
	sqlite3_exec(dbConnection, createParentTableSQL.c_str(), NULL, 0, &errorMessage);


	// initialize the result vector
	dbResults = vector<vector<string>>();
}

// method to close the database connection
void Database::close() {
	sqlite3_close(dbConnection);
}

void Database::insertProcedure(string name)
{
	string insertProcedureSQL = "INSERT INTO procedures ('name') VALUES ('" + name + "');";
	sqlite3_exec(dbConnection, insertProcedureSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Variable into the database
void Database::insertVariable(string name) {
	string insertVariablesSQL = "INSERT INTO variables ('name') VALUES ('" + name + "');";
	sqlite3_exec(dbConnection, insertVariablesSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Constant into the database
void Database::insertConstant(string value) {
	string insertConstantsSQL = "INSERT INTO constants ('value') VALUES ('" + value + "');";
	sqlite3_exec(dbConnection, insertConstantsSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Assignment into the database
void Database::insertAssignment(string stmtNo) {
	string insertAssignmentsSQL = "INSERT INTO assigns ('stmtNo') VALUES ('" + stmtNo + "');";
	sqlite3_exec(dbConnection, insertAssignmentsSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Print into the database
void Database::insertPrint(string stmtNo) {
	string insertPrintsSQL = "INSERT INTO prints ('stmtNo') VALUES ('" + stmtNo + "');";
	sqlite3_exec(dbConnection, insertPrintsSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Read into the database
void Database::insertRead(string stmtNo) {
	string insertReadsSQL = "INSERT INTO reads ('stmtNo') VALUES ('" + stmtNo + "');";
	sqlite3_exec(dbConnection, insertReadsSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Statement into the database
void Database::insertStmt(string stmtNo) {
	string insertStmtsSQL = "INSERT INTO stmts ('stmtNo') VALUES ('" + stmtNo + "');";
	sqlite3_exec(dbConnection, insertStmtsSQL.c_str(), NULL, 0, &errorMessage);
}

// method to insert a Next into the database
void Database::insertNext(string stmtNo, string nextStmtNo, string direct) {
	string insertStmtsSQL = "INSERT INTO nexts VALUES ('" + stmtNo + "' , '" + nextStmtNo + "', '" + direct + "');";
	sqlite3_exec(dbConnection, insertStmtsSQL.c_str(), NULL, 0, &errorMessage);
}



// method to get all the Procedures from the database
void Database::getProcedures(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the procedures from the procedure table
	// The callback method is only used when there are results to be returned.
	string getProceduresSQL = "SELECT * FROM procedures;";
	sqlite3_exec(dbConnection, getProceduresSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get all the Variable from the database
void Database::getVariables(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Variables from the Variables table
	// The callback method is only used when there are results to be returned.
	string getVariablesSQL = "SELECT * FROM variables;";
	sqlite3_exec(dbConnection, getVariablesSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get all the Constants from the database
void Database::getConstants(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Constants from the Constants table
	// The callback method is only used when there are results to be returned.
	string getConstantsSQL = "SELECT * FROM constants;";
	sqlite3_exec(dbConnection, getConstantsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}


// method to get all the Assignments from the database
void Database::getAssignments(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Assignments from the Assignments table
	// The callback method is only used when there are results to be returned.
	string getAssignmentsSQL = "SELECT * FROM assigns;";
	sqlite3_exec(dbConnection, getAssignmentsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get all the Prints from the database
void Database::getPrints(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Prints from the Prints table
	// The callback method is only used when there are results to be returned.
	string getPrintsSQL = "SELECT * FROM prints;";
	sqlite3_exec(dbConnection, getPrintsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get all the Reads from the database
void Database::getReads(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Reads from the Reads table
	// The callback method is only used when there are results to be returned.
	string getReadsSQL = "SELECT * FROM reads;";
	sqlite3_exec(dbConnection, getReadsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

// method to get all the Statements from the database
void Database::getStmts(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Statements from the Statements table
	// The callback method is only used when there are results to be returned.
	string getStmtsSQL = "SELECT * FROM stmts;";
	sqlite3_exec(dbConnection, getStmtsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

void Database::getNexts(vector<string>& results) {
	// clear the existing results
	dbResults.clear();

	// retrieve the Nexts from the nexts table
	// The callback method is only used when there are results to be returned.
	string getNextsSQL = "SELECT * FROM nexts;";
	sqlite3_exec(dbConnection, getNextsSQL.c_str(), callback, 0, &errorMessage);

	// postprocess the results from the database so that the output is just a vector of procedure names
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}


// callback method to put one row of results from the database into the dbResults vector
// This method is called each time a row of results is returned from the database
int Database::callback(void* NotUsed, int argc, char** argv, char** azColName) {
	NotUsed = 0;
	vector<string> dbRow;

	// argc is the number of columns for this row of results
	// argv contains the values for the columns
	// Each value is pushed into a vector.
	for (int i = 0; i < argc; i++) {
		dbRow.push_back(argv[i]);
	}

	// The row is pushed to the vector for storing all rows of results 
	dbResults.push_back(dbRow);

	return 0;
}
