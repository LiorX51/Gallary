#include <map>
#include <algorithm>

#include "ItemNotFoundException.h"
#include "MemoryAccess.h"
#include <string>
#include <vector>

void MemoryAccess::printAlbums() 
{
	std::list<Album> albums = getAlbums();
	for (std::list<Album>::iterator i = albums.begin(); i != albums.end(); i++)
	{
		std::cout << "Owner Id: " << i->getOwnerId() << ", Album Name: " + i->getName() << ", Date: " + i->getCreationDate() << std::endl;
	}
}

bool MemoryAccess::open()
{
	int res = sqlite3_open(BD_FILE_NAME, &this->db);
	char* errMessage = nullptr;

	if (res != SQLITE_OK)
	{
		this->db = nullptr;
		std::cout << "Failed to open DB" << std::endl;
		return false;
	}

	//create needed tables:

	// albums table;
	char* sqlStatement = "CREATE TABLE ALBUMS (\
		ID            INTEGER PRIMARY KEY\
		NOT NULL,\
		NAME          TEXT    NOT NULL,\
		CREATION_DATE TEXT    NOT NULL,\
		USER_ID       TEXT    NOT NULL\
		REFERENCES USERS(ID)\
		); ";

	res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return false;

	// users table;
	sqlStatement = "CREATE TABLE USERS (ID INT INT FOREIGN_KEY REFERENCES ALBUMS(USER_ID) NOT NULL, NAME TEXT NOT NULL); ";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return false;

	// tags table;
	sqlStatement = "	CREATE TABLE TAGS(\
		ID         INT PRIMARY KEY\
		NOT NULL,\
		PICTURE_ID TEXT  INT FOREIGN_KEY REFERENCES PICTURES(ID)  NOT NULL,\
		USER_ID    TEXT  INT FOREIGN_KEY REFERENCES USERS(ID)  NOT NULL\
		); ";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return false;

	// pictures table;
	sqlStatement = "CREATE TABLE PICTURES (\
		ID            INT PRIMARY KEY	NOT NULL,\
		NAME          TEXT    NOT NULL,\
		LOCATION      TEXT    NOT NULL,\
		CREATION_DATE TEXT    NOT NULL,\
		ALBUM_ID      INT	FOREIGN_KEY REFERENCES ALBUMS(ID)	  NOT NULL\
		);\
		";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return false;

	return true;
}

void MemoryAccess::close()
{
	if (this->db != nullptr)
	{
		sqlite3_close(this->db);
		db = nullptr;
	}
}

void MemoryAccess::clear()
{
}

auto MemoryAccess::getAlbumIfExists(const std::string & albumName)
{
	auto result = std::find_if(std::begin(m_albums), std::end(m_albums), [&](auto& album) { return album.getName() == albumName; });

	if (result == std::end(m_albums)) {
		throw ItemNotFoundException("Album not exists: ", albumName);
	}
	return result;

}

Album MemoryAccess::createDummyAlbum(const User& user)
{
	std::stringstream name("Album_" +std::to_string(user.getId()));

	Album album(user.getId(),name.str());

	for (int i=1; i<3; ++i)	{
		std::stringstream picName("Picture_" + std::to_string(i));

		Picture pic(i++, picName.str());
		pic.setPath("C:\\Pictures\\" + picName.str() + ".bmp");

		album.addPicture(pic);
	}

	return album;
}

const std::list<Album> MemoryAccess::getAlbums() 
{
	std::list<Album> albums;
	std::string sqlStatement = "SELECT * FROM Albums;";
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), albumCallback, &albums, &errMessage);
	return albums;
}

const std::list<Album> MemoryAccess::getAlbumsOfUser(const User& user) 
{	
	std::list<Album> albums;
	std::string sqlStatement = "SELECT * FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), albumCallback, &albums, &errMessage);
	return albums;
}

void MemoryAccess::createAlbum(const Album& album)
{
	std::string sqlStatement = "INSERT INTO ALBUMS (NAME, CREATION_DATE, USER_ID) VALUES ('" + album.getName() + "', '" +
		album.getCreationDate() + "', '" + std::to_string(album.getOwnerId()) + "');";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::deleteAlbum(const std::string& albumName, int userID)
{
	char* errMessage = nullptr;
	std::string sqlStatement;

	sqlStatement = "DELETE FROM TAGS WHERE PICTURE_ID = (SELECT ID FROM PICTURES WHERE ALBUM_ID = '(SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1)');";

	errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE Name = '" + albumName + "');";
	
	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID ='" + std::to_string(userID) + "';";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

bool MemoryAccess::doesAlbumExists(const std::string& albumName, int userID) 
{
	std::string sqlStatement = "SELECT COUNT(1) FROM ALBUMS WHERE NAME = '" + albumName + "' AND USER_ID = '" + std::to_string(userID) + "';";
	bool exist = false;
	char* errMessage = nullptr;
	
	int res = sqlite3_exec(db, sqlStatement.c_str(), doesExistCallBack, &exist, &errMessage);
		
	return exist;
}

Album MemoryAccess::openAlbum(const std::string& albumName) 
{
	Album r_album(0, "");

	char* errMessage = nullptr;
	std::list<Picture> pictures;
	std::vector<int> tags;

	std::string sqlStatement = "SELECT * FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1;";

	int res = sqlite3_exec(db, sqlStatement.c_str(), makeAlbumCallBack, &r_album, &errMessage);
	if (res != SQLITE_OK)
	{
		std::cout << "Can't find the album!";
	}

	sqlStatement = "SELECT * FROM PICTURES INNER JOIN ALBUMS ON PICTURES.ALBUM_ID = ALBUMS.ID WHERE ALBUMS.NAME = '" + r_album.getName() + "';";

	res = sqlite3_exec(db, sqlStatement.c_str(), picsCallback, &pictures, &errMessage);
	if (res != SQLITE_OK)
	{
		std::cout << "Can't find the pictures!";
	}

	for (std::list<Picture>::iterator i = pictures.begin(); i != pictures.end(); i++)
	{
		tags.clear();
		sqlStatement = "SELECT * FROM TAGS WHERE PICTURE_ID = '" + std::to_string(i->getId()) + "';";
		
		res = sqlite3_exec(db, sqlStatement.c_str(), tagsCallBack, &tags, &errMessage);
		if (res != SQLITE_OK)
		{
			std::cout << "Can't find the tags!";
		}

		for (int j = 0; j < tags.size(); j++)
		{
			i->tagUser(tags[j]);
		}
		r_album.addPicture(*i);
	}
	return r_album;
}

void MemoryAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture) 
{
	std::string sqlStatement = "INSERT INTO PICTURES VALUES('" + std::to_string(picture.getId()) + "', '" + picture.getName() + "', '" +
		picture.getPath() + "', '" + picture.getCreationDate() + "', (SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1));";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) 
{
	std::string sqlStatement;

	sqlStatement = "DELETE FROM TAGS WHERE PICTURE_ID = '(SELECT ID FROM PICTURES WHERE NAME = " + pictureName + " AND ALBUM_ID = '(SELECT ID FROM ALBUMS WHERE Name = " + albumName + " LIMIT 1)';";
	//std::string sqlStatement = "DELETE FROM TAGS WHERE PICTURE_ID = (SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "') AND ALBUM_ID = (SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "');";
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = '(SELECT ID FROM Albums WHERE Name = '" + albumName + "' LIMIT 1)'";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userID)
{
	std::string sqlStatement = "INSERT INTO TAGS(PICTURE_ID, USER_ID) VALUES(SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = '(SELECT ID FROM Albums WHERE Name = '" + albumName + "' LIMIT 1)', '" + std::to_string(userID) + "');";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userID)
{
	std::string sqlStatement = "DELETE FROM TAGS WHERE PICTURE_ID = '(SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = (SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1)' AND USER_ID = '" + std::to_string(userID) + "');";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::closeAlbum(Album& ) 
{
	// basically here we would like to delete the allocated memory we got from openAlbum
}

// ******************* User ******************* 
void MemoryAccess::printUsers()
{
	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	
	std::string sqlStatement = "SELECT * FROM USERS;";
	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), usersCallback, nullptr, &errMessage);
	if (res != SQLITE_OK)
		std::cout << "Error in getting users";
}

User MemoryAccess::getUser(int userID) 
{
	std::string name;
	char* errMessage = nullptr;

	std::string sqlStatement = "SELECT * FROM USERS WHERE ID = '" + std::to_string(userID) + "';";
	int res = sqlite3_exec(db, sqlStatement.c_str(), makeUserCallback, &name, &errMessage);

	return User(userID, name);
}

void MemoryAccess::createUser(User& user)
{
	std::string sqlStatement = "INSERT INTO USERS(ID, NAME)	VALUES('" + std::to_string(user.getId()) + "', '" + user.getName() + "');";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::deleteUser(const User& user)
{
	char* errMessage = nullptr;
	std::string sqlStatement;

	sqlStatement = "DELETE FROM TAGS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		std::cout << "couldn't delete tags";

	sqlStatement = "DELETE FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "');";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE FROM USERS WHERE ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

bool MemoryAccess::doesUserExists(int userID) 
{
	std::string sqlStatement = "SELECT COUNT(1) FROM USERS WHERE ID = '" + std::to_string(userID) + "';";
	bool exist = false;
	char* errMessage = nullptr;

	int res = sqlite3_exec(db, sqlStatement.c_str(), doesExistCallBack, &exist, &errMessage);
	if (res != SQLITE_OK)
		std::cout << "Couldn't find user";

	return exist;
}


// user statistics
int MemoryAccess::countAlbumsOwnedOfUser(const User& user) 
{
	int userAlbums = 0;
	char* errMessage = nullptr;

	std::string sqlStatement = "SELECT COUNT(*) FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";
	
	int res = sqlite3_exec(db, sqlStatement.c_str(), countCallback, &userAlbums, &errMessage);
	
	return userAlbums;
}

int MemoryAccess::countAlbumsTaggedOfUser(const User& user) 
{
	int numAlbums = 0;
	char* errMessage = nullptr;
	
	//found line on stack overflow: https://stackoverflow.com/a/12095198
	std::string sqlStatement = "SELECT COUNT(DISTINCT PICTURES.ALBUM_ID) FROM PICTURES, TAGS WHERE PICTURES.ID = TAGS.PICTURES_ID AND TAGS.USER_ID = '" + std::to_string(user.getId()) + "';";

	int res = sqlite3_exec(db, sqlStatement.c_str(), countCallback, &numAlbums, &errMessage);

	return numAlbums;
}

int MemoryAccess::countTagsOfUser(const User& user) 
{
	int userTag = 0;
	char* errMessage = nullptr;

	std::string sqlStatement = "SELECT COUNT(*) FROM TAGS WHERE USER_ID = " + std::to_string(user.getId()) + ";";

	int res = sqlite3_exec(db, sqlStatement.c_str(), countCallback, &userTag, &errMessage);

	return userTag;
}

float MemoryAccess::averageTagsPerAlbumOfUser(const User& user) 
{
	int albumsTaggedCount = countAlbumsTaggedOfUser(user);

	if ( 0 == albumsTaggedCount ) {
		return 0;
	}

	return static_cast<float>(countTagsOfUser(user)) / albumsTaggedCount;
}

User MemoryAccess::getTopTaggedUser()
{
	User maxUser(0, "");
	char* errMessage = nullptr;

	std::string sqlStatement = "SELECT USERS, NAME FROM TAGS INEER JOIN USERS ON USER_ID = USERS.ID GROUP BY USER_ID ORDER BY COUNT(*) DESC LIMIT 1;";
	
	int res = sqlite3_exec(db, sqlStatement.c_str(), maxTaggedUserCallback, &maxUser, &errMessage);

	return maxUser;
}

Picture MemoryAccess::getTopTaggedPicture()
{
	Picture maxPicture(0, "");
	char* errMessage = nullptr;
	
	std::string sqlStatement = "SELECT PICTURE_ID, NAME, CREATION_DATE FROM TAGS INNER JOIN PICTURES ON PICTURE_ID = PICTUR.ID GROUP BY PICTURE_ID ORDER BY COUNT(*) DESC LIMIT 1;";

	int res = sqlite3_exec(db, sqlStatement.c_str(), maxTaggedPicCallback, &maxPicture, &errMessage);
	
	return maxPicture;
}

std::list<Picture> MemoryAccess::getTaggedPicturesOfUser(const User& user)
{
	std::list<Picture> allUserPictures;
	std::vector<int> allTagedUsers;
	char* errMessage = nullptr;

	std::string sqlStatement = "SELECT PICTURE_ID, NAME, CREATION_DATE, LOCATION FROM TAGS INNER JOIN PICTURES ON PICTURE_ID = PICTURE_ID WHERE USER_ID = " + std::to_string(user.getId()) + ";";
	int res = sqlite3_exec(db, sqlStatement.c_str(), picsCallback, &allUserPictures, &errMessage);
	if (res != SQLITE_OK)
		std::cout << "error: cant find pictures";
	
	for (std::list<Picture>::iterator i = allUserPictures.begin(); i != allUserPictures.end(); i++)
	{
		allTagedUsers.clear();
		
		sqlStatement = "SELECT * FROM TAGS WHERE PICTURE_ID = " + std::to_string(i->getId()) + ";";
		
		res = sqlite3_exec(db, sqlStatement.c_str(), tagsCallBack, &allTagedUsers, &errMessage);
		
		if (res != SQLITE_OK)
			std::cout << "error: cant find tags";

		for (int j = 0; j < allTagedUsers.size(); j++)
		{
			i->tagUser(allTagedUsers[j]);
		}
	}
	return allUserPictures;
}

//callbacks:

// get albums
int MemoryAccess::albumCallback(void* data, int argc, char** argv, char** azColName)
{
	Album temp_album;
	std::list<Album>* albumList = static_cast<std::list<Album>*>(data);
	for (int i = 0; i < argc; i++)
	{
		if (std::string(azColName[i]) == "NAME")
		{
			temp_album.setName(argv[i]);
		}
		else if (std::string(azColName[i]) == "CREATION_DATE")
		{
			temp_album.setCreationDate(argv[i]);
		}
		else if (std::string(azColName[i]) == "USER_ID")
		{
			temp_album.setOwner(atoi(argv[i]));
		}
	}
	albumList->push_back(temp_album);
	return 0;
}

// does exist callback
int MemoryAccess::doesExistCallBack(void* data, int argc, char** argv, char** azColName)
{
	bool* found = static_cast<bool*>(data);
	if (*argv[0] != '0')
	{
		*found = true;
	}
	return 0;
}

// makes an album from the given data
int MemoryAccess::makeAlbumCallBack(void* data, int argc, char** argv, char** azColName)
{
	Album* r_album = static_cast<Album*>(data);
	for (int i = 0; i < argc; i++)
	{
		if (std::string(azColName[i]) == "USER_ID")
		{
			r_album->setOwner(atoi(argv[i]));
		}
		else if (std::string(azColName[i]) == "NAME")
		{
			r_album->setName(argv[i]);
		}
		else if (std::string(azColName[i]) == "CREATION_DATE")
		{
			r_album->setCreationDate(argv[i]);
		}
	}
	return 0;
}

// makes pictures from the given data
int MemoryAccess::picsCallback(void* data, int argc, char** argv, char** azColName)
{
	Picture r_pic(0, "");
	std::list<Picture>* picList = static_cast<std::list<Picture>*>(data);
	for (int i = 0; i < argc; i++)
	{
		if (std::string(azColName[i]) == "ID")
		{
			r_pic.setId(atoi(argv[i]));
		}
		else if (std::string(azColName[i]) == "NAME")
		{
			r_pic.setName(argv[i]);
		}
		else if (std::string(azColName[i]) == "CREATION_DATE")
		{
			r_pic.setCreationDate(argv[i]);
		}
		else if (std::string(azColName[i]) == "LOCATION")
		{
			r_pic.setPath(argv[i]);
		}
		else if (std::string(azColName[i]) == "ALBUM_ID")
		{
			picList->push_back(r_pic);
		}
	}
	return 0;
}

// makes the tags from given data
int MemoryAccess::tagsCallBack(void* data, int argc, char** argv, char** azColName)
{
	std::vector<int>* tags = static_cast<std::vector<int>*>(data);
	for (int i = 0; i < argc; i++)
	{
		if (std::string(azColName[i]) == "USER_ID")
		{
			tags->push_back(atoi(argv[i]));
		}
	}
	return 0;
}

int MemoryAccess::makeUserCallback(void* data, int argc, char** argv, char** azColName)
{
	std::string* name = static_cast<std::string*>(data);
	
	if (std::string(azColName[1]) == "NAME")
	{
		*name = argv[1];
	}

	return 0;
}

int MemoryAccess::countCallback(void* data, int argc, char** argv, char** azColName)
{
	int* count = static_cast<int*>(data);
	*count = atoi(argv[0]);

	return 0;
}

int MemoryAccess::maxTaggedUserCallback(void* data, int argc, char** argv, char** azColName)
{
	User* maxUser = static_cast<User*>(data);

	maxUser->setId(atoi(argv[0]));
	maxUser->setName(argv[1]);

	return 0;
}

int MemoryAccess::maxTaggedPicCallback(void* data, int argc, char** argv, char** azColName)
{
	Picture* maxTaggedPic = static_cast<Picture*>(data);

	maxTaggedPic->setId(atoi(argv[0]));
	maxTaggedPic->setName(argv[1]);
	maxTaggedPic->setCreationDate(argv[2]);
	return 0;
}

int MemoryAccess::usersCallback(void* data, int argc, char** argv, char** azColName)
{
	for (int i = 0; i < argc; i++)
	{
		std::cout << azColName[i] << ": " << argv[i] << ", ";
	}
	std::cout << std::endl;
	return 0;
}