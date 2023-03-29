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
	executeQuery("CREATE TABLE variable ( procedureName VARCHAR(255), name VARCHAR(255), stmtNo VARCHAR(255), PRIMARY KEY(procedureName, name, stmtNo));");

	// drop/create the existing constant table (if any)
	executeQuery("DROP TABLE IF EXISTS constant");
	executeQuery("CREATE TABLE constant ( value VARCHAR(255), stmtNo VARCHAR(255), PRIMARY KEY(value, stmtNo));");

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
	executeQuery("CREATE TABLE while ( stmtNo VARCHAR(255) , parentStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', isParent VARCHAR(255) DEFAULT '0', PRIMARY KEY(stmtNo, parentStmtNo));");

	// drop/create the existing if table (if any)
	executeQuery("DROP TABLE IF EXISTS if_table");
	executeQuery("CREATE TABLE if_table ( stmtNo VARCHAR(255) , parentStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', isParent VARCHAR(255) DEFAULT '0', PRIMARY KEY(stmtNo, parentStmtNo));");

	// drop/create the existing pattern table (if any)
	executeQuery("DROP TABLE IF EXISTS pattern_table");
	executeQuery("CREATE TABLE pattern_table ( stmtNo VARCHAR(255), source VARCHAR(255), target VARCHAR(255), PRIMARY KEY (stmtNo, source, target));");

	// drop the existing next table (if any)
	executeQuery("DROP TABLE IF EXISTS nexts");
	executeQuery("CREATE TABLE nexts ( stmtNo VARCHAR(255), nextStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, nextStmtNo)); ");

	// drop the existing parent table (if any)
	executeQuery("DROP TABLE IF EXISTS parents");
	executeQuery("CREATE TABLE parents ( stmtNo VARCHAR(255), parentStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', isFirst VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, parentStmtNo));");

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

string generateInsertQuery(string tableName, vector<string> columnNames, vector<string> values) {
	string sb = "INSERT INTO ";
	sb = sb + tableName;
	sb = sb + "(";
	for (string columnName : columnNames) {
		sb = sb + "'";
		sb = sb + columnName;
		sb = sb + "'";
		sb = sb + ",";
	}
	sb.pop_back();
	sb = sb + ") VALUES (";
	for (string value : values) {
		sb = sb + "'";
		sb = sb + value;
		sb = sb + "'";
		sb = sb + ",";
	}
	sb.pop_back();
	sb = sb + ");";
	return sb;
}

void Database::insertProcedure(string name)
{
	Database::executeQuery(generateInsertQuery("procedure", { "procedureName" }, { name }));
}

// method to insert a Variable into the database
void Database::insertVariable(string procedureName, string name, string stmtNo) {
	Database::executeQuery(generateInsertQuery("variable", { "procedureName", "name", "stmtNo"}, {procedureName, name, stmtNo}));
}

// method to insert a Constant into the database
void Database::insertConstant(string value, string stmtNo) {
	Database::executeQuery(generateInsertQuery("constant", { "value", "stmtNo" }, {value, stmtNo}));
}

// method to insert a Assignment into the database
void Database::insertAssignment(string stmtNo) {
	Database::executeQuery(generateInsertQuery("assign", { "stmtNo" }, { stmtNo }));
}

// method to insert a Print into the database
void Database::insertPrint(string stmtNo) {
	Database::executeQuery(generateInsertQuery("print", { "stmtNo" }, { stmtNo }));
}

// method to insert a Read into the database
void Database::insertRead(string stmtNo) {
	Database::executeQuery(generateInsertQuery("read", { "stmtNo" }, { stmtNo }));
}

// method to insert a Statement into the database
void Database::insertStmt(string stmtNo) {
	Database::executeQuery(generateInsertQuery("stmt", { "stmtNo" }, { stmtNo }));
}

// method to insert a while into the database
void Database::insertWhile(string stmtNo, string isParent, string parentStmtNo, string direct) {
	Database::executeQuery(generateInsertQuery("while", { "stmtNo", "isParent", "parentStmtNo", "direct" }, { stmtNo, isParent, parentStmtNo, direct }));
}

// method to insert a if into the database
void Database::insertIf(string stmtNo, string isParent, string parentStmtNo, string direct) {
	Database::executeQuery(generateInsertQuery("if_table", { "stmtNo", "isParent", "parentStmtNo", "direct" }, {stmtNo, isParent, parentStmtNo, direct }));
}

// method to insert a pattern into the database
void Database::insertPattern(string stmtNo, string source, string target) {
	Database::executeQuery(generateInsertQuery("pattern_table", { "stmtNo", "source", "target" }, { stmtNo, source, target }));
}

// method to insert a modifies into the database
void Database::insertModifies(string stmtNo, string procedureName, string target) {
	Database::executeQuery(generateInsertQuery("modifies", { "stmtNo", "procedureName", "target" }, { stmtNo, procedureName, target }));
}

// method to insert a uses into the database
void Database::insertUses(string stmtNo, string procedureName, string target) {
	Database::executeQuery(generateInsertQuery("uses", { "stmtNo", "procedureName", "target" }, { stmtNo, procedureName, target }));
}

// method to insert a Next into the database
void Database::insertNext(string stmtNo, string nextStmtNo, string direct) {
	Database::executeQuery(generateInsertQuery("nexts", { "stmtNo", "nextStmtNo", "direct" }, { stmtNo, nextStmtNo, direct }));
}

// method to insert a Parent into the database
void Database::insertParent(string stmtNo, string parentStmtNo, string direct, string isFirst) {
	Database::executeQuery(generateInsertQuery("parents", { "stmtNo", "parentStmtNo", "direct", "isFirst"}, {stmtNo, parentStmtNo, direct, isFirst}));
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
