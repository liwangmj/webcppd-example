#include <Poco/LocalDateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/Net/NameValueCollection.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/MessageHeader.h>
#include <Poco/Net/HTTPMessage.h>
#include <Poco/Path.h>
#include <Poco/Format.h>
#include <Poco/File.h>
#include <Poco/Net/HTTPCookie.h>
#include <Poco/MD5Engine.h>
#include <Poco/FileStream.h>
#include <Poco/TemporaryFile.h>
#include <Poco/String.h>


#include "uploadPartHandler.hpp"


namespace webcpp {

	uploadPartHandler::uploadPartHandler(const std::string allowName, const std::string allowType, const std::string uploadDirectory, double allowMaxSize)
	: data(), allowName(allowName), allowType(allowType), directory(uploadDirectory), allowMaxSize(allowMaxSize)
	{

	}

	void uploadPartHandler::handlePart(const Poco::Net::MessageHeader& header, std::istream& stream)
	{

		Poco::StringTokenizer nameST(this->allowName, "|", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY)
			, typeST(this->allowType, "|", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);


		std::string type = header.get("Content-Type");
		std::string disposition = header.get("Content-Disposition");
		std::string v;
		Poco::Net::NameValueCollection nvc;
		Poco::Net::HTTPMessage::splitParameters(disposition, v, nvc);

		webcpp::uploadPartHandler::fileinfo fileinfo;

		fileinfo.filename = Poco::Net::HTTPCookie::escape(nvc.get("filename"));
		fileinfo.name = Poco::Net::HTTPCookie::escape(nvc.get("name"));
		fileinfo.type = type;
		fileinfo.ok = false;
		fileinfo.message = "Not allow upload.";
		fileinfo.savepath = "";
		fileinfo.webpath = "";
		fileinfo.size = 0;

		if (typeST.has(fileinfo.type) && nameST.has(fileinfo.name)) {

			Poco::LocalDateTime now;
			fileinfo.savepath = Poco::format("%[0]s/%[1]s"
				, this->directory
				, Poco::DateTimeFormatter::format(now, "%Y/%m/%d/%H"));
			Poco::File uploaddir(fileinfo.savepath);
			if (!uploaddir.exists()) {
				uploaddir.createDirectories();
			}

			Poco::Path path(fileinfo.filename);
			Poco::MD5Engine md5;
			md5.update(Poco::DateTimeFormatter::format(now, Poco::DateTimeFormat::HTTP_FORMAT) + fileinfo.filename);
			fileinfo.savepath += Poco::format("/%[0]s.%[1]s", Poco::MD5Engine::digestToHex(md5.digest()), path.getExtension());

			//			Poco::TemporaryFile tempFile;
			Poco::FileOutputStream fileoutstream(fileinfo.savepath);


			bool allowSave = true;
			int ch = stream.get();
			double size(0);

			while (allowSave && ch >= 0) {
				if (size <= this->allowMaxSize) {
					fileoutstream.put((char) ch);
					ch = stream.get();
					size += 1;

				} else {
					allowSave = false;
					fileinfo.message += " The file size is too big.";
				}
			}

			if (allowSave) {
				fileinfo.webpath = Poco::replace(fileinfo.savepath, this->directory, std::string());
				fileinfo.size = size;
				fileinfo.ok = true;
				fileinfo.message = "Allow upload.";
			} else {
				Poco::File tempFile(fileinfo.savepath);
				tempFile.remove();
			}
		}
		this->data.push_back(fileinfo);

	}

	std::vector<webcpp::uploadPartHandler::fileinfo> uploadPartHandler::getData()
	{
		return this->data;
	}



}
