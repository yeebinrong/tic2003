#include "Database.h"
#include <iostream>

sqlite3* Database::dbConnection;
vector<vector<string>> Database::dbResults;
char* Database::errorMessage;

void Database::executeQuery(string query) {
	sqlite3_exec(dbConnection, query.c_str(), NULL, 0, &errorMessage);
}

// method to connect to the database and initialize tables in the database
void Database::initialize() {
	// open a database connection and store the pointer into dbConnection
	sqlite3_open("database.db", &dbConnection);

	// drop/create the existing procedure table (if any)
	executeQuery("DROP TABLE IF EXISTS procedure");
	executeQuery("CREATE TABLE procedure ( procedureName VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing variable table (if any)
	executeQuery("DROP TABLE IF EXISTS variable");
	executeQuery("CREATE TABLE variable ( name VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing constant table (if any)
	executeQuery("DROP TABLE IF EXISTS constant");
	executeQuery("CREATE TABLE constant ( value VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing assign table (if any)
	executeQuery("DROP TABLE IF EXISTS assign");
	executeQuery("CREATE TABLE assign ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing print table (if any)
	executeQuery("DROP TABLE IF EXISTS print");
	executeQuery("CREATE TABLE print ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing read table (if any)
	executeQuery("DROP TABLE IF EXISTS read");
	executeQuery("CREATE TABLE read ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing stmt table (if any)
	executeQuery("DROP TABLE IF EXISTS stmt");
	executeQuery("CREATE TABLE stmt ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing while table (if any)
	executeQuery("DROP TABLE IF EXISTS while");
	executeQuery("CREATE TABLE while ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing if table (if any)
	executeQuery("DROP TABLE IF EXISTS if_table");
	executeQuery("CREATE TABLE if_table ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing pattern table (if any)
	executeQuery("DROP TABLE IF EXISTS pattern_table");
	executeQuery("CREATE TABLE pattern_table ( stmtNo VARCHAR(255), source VARCHAR(255), target VARCHAR(255), PRIMARY KEY (stmtNo, source, target));");

	// drop/create the existing modifies table (if any)
	executeQuery("DROP TABLE IF EXISTS modifies");
	executeQuery("CREATE TABLE modifies ( stmtNo VARCHAR(255), procedureName VARCHAR(255), target VARCHAR(255), PRIMARY KEY (stmtNo, procedureName, target))");

	// drop/create the existing uses table (if any)
	executeQuery("DROP TABLE IF EXISTS uses");
	executeQuery("CREATE TABLE uses ( stmtNo VARCHAR(255), procedureName VARCHAR(255), target VARCHAR(255), PRIMARY KEY (stmtNo, procedureName, target))");

	// initialize the result vector
	dbResults = vector<vector<string>>();
}

// method to close the database connection
void Database::close() {
	sqlite3_close(dbConnection);
}

void Database::insertProcedure(string name)
{
	Database::executeQuery("INSERT INTO procedure ('procedureName') VALUES ('" + name + "');");
}

// method to insert a Variable into the database
void Database::insertVariable(string name) {
	Database::executeQuery("INSERT INTO variable ('name') VALUES ('" + name + "');");
}

// method to insert a Constant into the database
void Database::insertConstant(string value) {
	Database::executeQuery("INSERT INTO constant ('value') VALUES ('" + value + "');");
}

// method to insert a Assignment into the database
void Database::insertAssignment(string stmtNo) {
	Database::executeQuery("INSERT INTO assign ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Print into the database
void Database::insertPrint(string stmtNo) {
	Database::executeQuery("INSERT INTO print ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Read into the database
void Database::insertRead(string stmtNo) {
	Database::executeQuery("INSERT INTO read ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Statement into the database
void Database::insertStmt(string stmtNo) {
	Database::executeQuery("INSERT INTO stmt ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a while into the database
void Database::insertWhile(string stmtNo) {
	Database::executeQuery("INSERT INTO while ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a if into the database
void Database::insertIf(string stmtNo) {
	Database::executeQuery("INSERT INTO if_table ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a pattern into the database
void Database::insertPattern(string stmtNo, string source, string target) {
	Database::executeQuery("INSERT INTO pattern_table ('stmtNo', 'source', 'target') VALUES ('" + stmtNo + "', '" + source + "', '" + target + "'); ");
}

// method to insert a modifies into the database
void Database::insertModifies(string stmtNo, string procedureName, string target) {
	Database::executeQuery("INSERT INTO modifies ('stmtNo', 'procedureName', 'target') VALUES ('" + stmtNo + "', '" + procedureName + "', '" + target + "'); ");
}

// method to insert a uses into the database
void Database::insertUses(string stmtNo, string procedureName, string target) {
	Database::executeQuery("INSERT INTO uses ('stmtNo', 'procedureName', 'target') VALUES ('" + stmtNo + "', '" + procedureName + "', '" + target + "'); ");
}

void Database::executeQueryAndMapResults(vector<string>& results, string query) {
	// clear the existing results
	dbResults.clear();
	// the callback method is only used when there are results to be returned.
	sqlite3_exec(dbConnection, query.c_str(), callback, 0, &errorMessage);
	// postprocess the results from the database so that the output is just a vector of string
	for (vector<string> dbRow : dbResults) {
		string result;
		result = dbRow.at(0);
		results.push_back(result);
	}
}

void Database::getQueryResults(vector<string>& results, string queryToExecute) {
	Database::executeQueryAndMapResults(results, queryToExecute);
}

// method to get all the Procedures from the database
void Database::getProcedures(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT procedureName FROM procedure;");
}

// method to get all the Variable from the database
void Database::getVariables(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT name FROM variable;");
}

// method to get all the Constants from the database
void Database::getConstants(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT value FROM constant;");
}

// method to get all the Assignments from the database
void Database::getAssignments(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM assign;");
}

// method to get all the Prints from the database
void Database::getPrints(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM print;");
}

// method to get all the Reads from the database
void Database::getReads(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM read;");
}

// method to get all the Statements from the database
void Database::getStmts(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM stmt;");
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
