#include "Database.h"

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
	executeQuery("DROP TABLE IF EXISTS procedures");
	executeQuery("CREATE TABLE procedures ( name VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing variable table (if any)
	executeQuery("DROP TABLE IF EXISTS variables");
	executeQuery("CREATE TABLE variables ( name VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing constant table (if any)
	executeQuery("DROP TABLE IF EXISTS constants");
	executeQuery("CREATE TABLE constants ( value VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing assign table (if any)
	executeQuery("DROP TABLE IF EXISTS assigns");
	executeQuery("CREATE TABLE assigns ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing print table (if any)
	executeQuery("DROP TABLE IF EXISTS prints");
	executeQuery("CREATE TABLE prints ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing read table (if any)
	executeQuery("DROP TABLE IF EXISTS reads");
	executeQuery("CREATE TABLE reads ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop/create the existing stmt table (if any)
	executeQuery("DROP TABLE IF EXISTS stmts");
	executeQuery("CREATE TABLE stmts ( stmtNo VARCHAR(255) PRIMARY KEY);");

	// drop the existing next table (if any)
	executeQuery("DROP TABLE IF EXISTS nexts");
	executeQuery("CREATE TABLE nexts ( stmtNo VARCHAR(255), nextStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, nextStmtNo));");

	// drop the existing parent table (if any)
	executeQuery("DROP TABLE IF EXISTS parents");
	executeQuery("CREATE TABLE parents ( stmtNo VARCHAR(255), childStmtNo VARCHAR(255), direct VARCHAR(255) DEFAULT '0', CHECK (direct == '0' OR direct == '1'), PRIMARY KEY(stmtNo, childStmtNo));");

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
	Database::executeQuery("INSERT INTO procedures ('name') VALUES ('" + name + "');");
}

// method to insert a Variable into the database
void Database::insertVariable(string name) {
	Database::executeQuery("INSERT INTO variables ('name') VALUES ('" + name + "');");
}

// method to insert a Constant into the database
void Database::insertConstant(string value) {
	Database::executeQuery("INSERT INTO constants ('value') VALUES ('" + value + "');");
}

// method to insert a Assignment into the database
void Database::insertAssignment(string stmtNo) {
	Database::executeQuery("INSERT INTO assigns ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Print into the database
void Database::insertPrint(string stmtNo) {
	Database::executeQuery("INSERT INTO prints ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Read into the database
void Database::insertRead(string stmtNo) {
	Database::executeQuery("INSERT INTO reads ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a Statement into the database
void Database::insertStmt(string stmtNo) {
	Database::executeQuery("INSERT INTO Stmts ('stmtNo') VALUES ('" + stmtNo + "');");
}

// method to insert a modifies into the database
void Database::insertModifies(string stmtNo, string procedureName, string target) {
	Database::executeQuery("INSERT INTO modifies ('stmtNo', 'procedureName', 'target') VALUES ('" + stmtNo + "', '" + procedureName + "', '" + target + "'); ");
}

// method to insert a uses into the database
void Database::insertUses(string stmtNo, string procedureName, string target) {
	Database::executeQuery("INSERT INTO uses ('stmtNo', 'procedureName', 'target') VALUES ('" + stmtNo + "', '" + procedureName + "', '" + target + "'); ");
}

// method to insert a Next into the database
void Database::insertNext(string stmtNo, string nextStmtNo, string direct) {
	Database::executeQuery("INSERT INTO nexts ('stmtNo', 'nextStmtNo', 'direct') VALUES ('" + stmtNo + "', '" + nextStmtNo + "', '" + direct + "'); ");
}

// method to insert a Parent into the database
void Database::insertParent(string stmtNo, string childStmtNo, string direct) {
	Database::executeQuery("INSERT INTO parents ('stmtNo', 'childStmtNo', 'direct') VALUES ('" + stmtNo + "', '" + childStmtNo + "', '" + direct + "'); ");
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

// method to get all the Procedures from the database
void Database::getProcedures(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT name FROM procedures;");
}

// method to get all the Variable from the database
void Database::getVariables(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT name FROM variables;");
}

// method to get all the Constants from the database
void Database::getConstants(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT value FROM constants;");
}

// method to get all the Assignments from the database
void Database::getAssignments(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM assigns;");
}

// method to get all the Prints from the database
void Database::getPrints(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM prints;");
}

// method to get all the Reads from the database
void Database::getReads(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM reads;");
}

// method to get all the Statements from the database
void Database::getStmts(vector<string>& results) {
	Database::executeQueryAndMapResults(results, "SELECT stmtNo FROM stmts;");
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
