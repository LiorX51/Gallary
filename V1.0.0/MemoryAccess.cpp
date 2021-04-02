#include <map>
#include <algorithm>

#include "ItemNotFoundException.h"
#include "MemoryAccess.h"
#include <string>

void MemoryAccess::printAlbums() 
{
	if(m_albums.empty()) {
		throw MyException("There are no existing albums.");
	}
	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const Album& album: m_albums) 	{
		std::cout << std::setw(5) << "* " << album;
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
	char* sqlStatement = "CREATE TABLE ALBUMS (ID INT PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, CREATION_DATE TEXT NOT NULL, USER_ID TEXT NOT NULL); ";

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
		ID            INT PRIMARY KEY\
		NOT NULL,\
		NAME          TEXT    NOT NULL,\
		LOCATION      TEXT    NOT NULL,\
		CREATION_DATE TEXT    NOT NULL,\
		ALBUM_ID      INT	INT FOREIGN_KEY REFERENCES ALBUMS(ID)	  NOT NULL\
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
	std::list<Album> albumsOfUser;
	for (const auto& album: m_albums) {
		if (album.getOwnerId() == user.getId()) {
			albumsOfUser.push_back(album);
		}
	}
	return albumsOfUser;
}

void MemoryAccess::createAlbum(const Album& album)
{
	char* errMessage = nullptr;
	char* sqlStatement;

	std::string id = std::to_string(album.getOwnerId());	
	std::string name = album.getName();
	
	sqlStatement = "INSERT INTO ALBUMS (NAME, CREATION_DATE, USER_ID) VALUES(", name.c_str(), ", " + album.getCreationDate() + ", " + id + ");";

	errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement, nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::deleteAlbum(const std::string& albumName, int userID)
{
	char* errMessage = nullptr;
	std::string sqlStatement;

	sqlStatement = "DELETE * FROM TAGS WHERE PICTURE_ID = (SELECT ID FROM PICTURES WHERE ALBUM_ID = '(SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1)');";

	errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE * FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE Name = '" + albumName + "');";
	
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
	for (const auto& album: m_albums) {
		if ( (album.getName() == albumName) && (album.getOwnerId() == userID) ) {
			return true;
		}
	}

	return false;
}

Album MemoryAccess::openAlbum(const std::string& albumName) 
{
	for (auto& album: m_albums)	{
		if (albumName == album.getName()) {
			return album;
		}
	}
	throw MyException("No album with name " + albumName + " exists");
}

void MemoryAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture) 
{
	std::string sqlStatement = "INSERT INTO Pictures VALUES(" + std::to_string(picture.getId()) + ", '" + picture.getName() + "', '" + picture.getPath() + "', '" + picture.getCreationDate() + "', (SELECT ID FROM Albums WHERE Name = '" + albumName + "' LIMIT 1));";

	char* errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

void MemoryAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) 
{
	std::string sqlStatement;

	sqlStatement = "DELETE FROM TAGS WHERE PICTURE_ID = '(SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = '(SELECT ID FROM Albums WHERE Name = '" + albumName + "' LIMIT 1)'";

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
	std::string sqlStatement = "DELETE * FROM TAGS WHERE PICTURE_ID = '(SELECT ID FROM PICTURES WHERE NAME = '" + pictureName + "' AND ALBUM_ID = (SELECT ID FROM ALBUMS WHERE NAME = '" + albumName + "' LIMIT 1)' AND USER_ID = '" + std::to_string(userID) + "');";

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
	for (const auto& user: m_users) {
		std::cout << user << std::endl;
	}
}

User MemoryAccess::getUser(int userID) {
	for (const auto& user : m_users) {
		if (user.getId() == userID) {
			return user;
		}
	}

	throw ItemNotFoundException("User", userID);
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

	sqlStatement = "DELETE * FROM TAGS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	int res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE * FROM PICTURES WHERE ALBUM_ID = (SELECT ID FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "');";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE * FROM ALBUMS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;

	sqlStatement = "DELETE * FROM USERS WHERE USER_ID = '" + std::to_string(user.getId()) + "';";

	errMessage = nullptr;
	res = sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errMessage);
	if (res != SQLITE_OK)
		return;
}

bool MemoryAccess::doesUserExists(int userID) 
{
	auto iter = m_users.begin();
	for (const auto& user : m_users) {
		if (user.getId() == userID) {
			return true;
		}
	}
	
	return false;
}


// user statistics
int MemoryAccess::countAlbumsOwnedOfUser(const User& user) 
{
	int albumsCount = 0;

	for (const auto& album: m_albums) {
		if (album.getOwnerId() == user.getId()) {
			++albumsCount;
		}
	}

	return albumsCount;
}

int MemoryAccess::countAlbumsTaggedOfUser(const User& user) 
{
	int albumsCount = 0;

	for (const auto& album: m_albums) {
		const std::list<Picture>& pics = album.getPictures();
		
		for (const auto& picture: pics)	{
			if (picture.isUserTagged(user))	{
				albumsCount++;
				break;
			}
		}
	}

	return albumsCount;
}

int MemoryAccess::countTagsOfUser(const User& user) 
{
	int tagsCount = 0;

	for (const auto& album: m_albums) {
		const std::list<Picture>& pics = album.getPictures();
		
		for (const auto& picture: pics)	{
			if (picture.isUserTagged(user))	{
				tagsCount++;
			}
		}
	}

	return tagsCount;
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
	std::map<int, int> userTagsCountMap;

	auto albumsIter = m_albums.begin();
	for (const auto& album: m_albums) {
		for (const auto& picture: album.getPictures()) {
			
			const std::set<int>& userTags = picture.getUserTags();
			for (const auto& user: userTags ) {
				//As map creates default constructed values, 
				//users which we haven't yet encountered will start from 0
				userTagsCountMap[user]++;
			}
		}
	}

	if (userTagsCountMap.size() == 0) {
		throw MyException("There isn't any tagged user.");
	}

	int topTaggedUser = -1;
	int currentMax = -1;
	for (auto entry: userTagsCountMap) {
		if (entry.second < currentMax) {
			continue;
		}

		topTaggedUser = entry.first;
		currentMax = entry.second;
	}

	if ( -1 == topTaggedUser ) {
		throw MyException("Failed to find most tagged user");
	}

	return getUser(topTaggedUser);
}

Picture MemoryAccess::getTopTaggedPicture()
{
	int currentMax = -1;
	const Picture* mostTaggedPic = nullptr;
	for (const auto& album: m_albums) {
		for (const Picture& picture: album.getPictures()) {
			int tagsCount = picture.getTagsCount();
			if (tagsCount == 0) {
				continue;
			}

			if (tagsCount <= currentMax) {
				continue;
			}

			mostTaggedPic = &picture;
			currentMax = tagsCount;
		}
	}
	if ( nullptr == mostTaggedPic ) {
		throw MyException("There isn't any tagged picture.");
	}

	return *mostTaggedPic;	
}

std::list<Picture> MemoryAccess::getTaggedPicturesOfUser(const User& user)
{
	std::list<Picture> pictures;

	for (const auto& album: m_albums) {
		for (const auto& picture: album.getPictures()) {
			if (picture.isUserTagged(user)) {
				pictures.push_back(picture);
			}
		}
	}

	return pictures;
}


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