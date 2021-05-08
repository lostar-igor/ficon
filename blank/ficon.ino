//=======================================================================================
// FiCon v1.00 by lostar-igor                                                         ==
// Файловый менеджер для ESP8266                                                      ==
//=======================================================================================

// Вывод отладочной информации в монитор
// Если нет необходимости - закомментировать
#define ficonDBG  // отладка

// Определение порта для вывода в монитор, если не определено ранее
#ifdef ficonDBG
  #ifndef ficonDbgSerial
    #define ficonDbgSerial Serial
  #endif
#endif
/*
// Инициализация библиотеки WEB-сервера, если не проинициализирована ранее
#ifndef ficonHttpServer
  // Библиотека WEB-сервера
  #include <ESP8266WebServer.h>
  // Инициализация библиотеки WEB-сервера
  ESP8266WebServer ficonHttpServer(80);
#endif
*/
// Подготовка к инициализации файловой системы SPIFFS, если не проинициализирована ранее
#ifndef ficonFS
  // Результат инициализации файловой системы
  static bool ficonFSOK;
  #define ficonFS SPIFFS
  // Необходимость в инициализации файловой системы
  #define ficonInitFS
  // Подключение библиотеки для работы с SPIFFS
  #include "FS.h"
#endif

// Глобальные переменные
File uploadFile;

// Константы
static const char TEXT_PLAIN[] PROGMEM = "text/plain";
static const char TEXT_JSON[] PROGMEM = "text/json";
static const char ACCESS_CAO[] PROGMEM = "\r\nAccess-Control-Allow-Origin: *";
static const char FS_INIT_ERROR[] PROGMEM = "FS INIT ERROR";
//static const char BAD_PATH[] PROGMEM = "BAD PATH";
static const char BAD_SRC[] PROGMEM = "BAD SRC";
static const char CREATE_FAILED[] PROGMEM = "CREATE FAILED";
static const char WRITE_FAILED[] PROGMEM = "WRITE FAILED";
static const char RENAME_FAILED[] PROGMEM = "RENAME FAILED";
static const char FILE_NOT_FOUND[] PROGMEM = "FILE NOT FOUND";
static const char SRC_FILE_NOT_FOUND[] PROGMEM = "SRC FILE NOT FOUND";
static const char FILE_EXISTS[] PROGMEM = "FILE EXISTS";
static const char DELETE_ERROR[] PROGMEM = "DELETE ERROR";
static const char DST_NAME_ARG_MISSING[] PROGMEM = "dstName ARG MISSING";

//=======================================================================================
// Функция определения типа контента по расширению файла                               ==
//=======================================================================================
String getContentType(String filename){
  if(ficonHttpServer.hasArg("download")) return F("application/octet-stream");
  else if(filename.endsWith(".htm")) return F("text/html");
  else if(filename.endsWith(".html")) return F("text/html");
  else if(filename.endsWith(".css")) return F("text/css");
  else if(filename.endsWith(".js")) return F("application/javascript");
  else if(filename.endsWith(".png")) return F("image/png");
  else if(filename.endsWith(".gif")) return F("image/gif");
  else if(filename.endsWith(".jpg")) return F("image/jpeg");
  else if(filename.endsWith(".ico")) return F("image/x-icon");
  else if(filename.endsWith(".xml")) return F("text/xml");
  else if(filename.endsWith(".pdf")) return F("application/x-pdf");
  else if(filename.endsWith(".zip")) return F("application/x-zip");
  else if(filename.endsWith(".gz")) return F("application/x-gzip");
  return FPSTR(TEXT_PLAIN);
}

//=======================================================================================
//== Функции возврата HTTP-кодов                                                       ==
//=======================================================================================
//Ок
void replyOK() {
  String msgType = FPSTR(TEXT_PLAIN);
  ficonHttpServer.send(200, msgType + FPSTR(ACCESS_CAO), "");
}
// Удачное выполнение запроса с возвратом сообщения
void replyOKWithMsg(String msg, String msgType = FPSTR(TEXT_PLAIN)) {
  ficonHttpServer.send(200, msgType + FPSTR(ACCESS_CAO), msg);
}
// Не найлено
void replyNotFound(String msg) {
  #ifdef ficonDBG
    Serial.println(msg);
  #endif
  String msgType = FPSTR(TEXT_PLAIN);
//  ficonHttpServer.send(404, FPSTR(TEXT_PLAIN), msg);
  ficonHttpServer.send(404, msgType + FPSTR(ACCESS_CAO), msg);
}
// Некорректный запрос
void replyBadRequest(String msg) {
  #ifdef ficonDBG
    Serial.println(msg);
  #endif
  String msgType = FPSTR(TEXT_PLAIN);
//  ficonHttpServer.send(400, FPSTR(TEXT_PLAIN), msg + "\r\n");
  ficonHttpServer.send(400, msgType + FPSTR(ACCESS_CAO), msg + "\r\n");
}
// Ошибка выполнения запроса
void replyServerError(String msg) {
  #ifdef ficonDBG
    ficonDbgSerial.println(msg);
  #endif
  ficonHttpServer.send(500, FPSTR(TEXT_PLAIN), msg + "\r\n");
}

//=======================================================================================
//== Проверка имени файла на наличие символов или комбинаций символов, которые         ==
//== не поддерживаются FiCon.                                                          ==
//== Возвращает пустую строку, если поддерживается, или сведения об ошибках,           ==
//== если не поддерживаются.                                                           ==
//=======================================================================================
String checkForUnsupportedPath(String filename) {
  String error = String();
  if (!filename.startsWith("/")) {
    error += F("!NO_LEADING_SLASH! ");
  }
  if (filename.indexOf("//") != -1) {
    error += F("!DOUBLE_SLASH! ");
  }
  if (filename.endsWith("/")) {
    error += F("!TRAILING_SLASH! ");
  }
  return error;
}

//=======================================================================================
//== Формирование JSON с информацией о файлах находящихся в SPIFFS ESP8266             ==
//=======================================================================================
void ficonFiles() {
  if (!ficonFSOK) { // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  #ifdef ficonDBG
    ficonDbgSerial.println();
    ficonDbgSerial.println(F("== Files in SPIFFS: =="));
  #endif

  Dir dir = ficonFS.openDir("/");

  String message = "[";
  while (dir.next()) {
    File entry = dir.openFile("r");
    if (message != "[") message += ",";
    bool isDir = false;
    message += F("{\"name\":\"");
    message += String(dir.fileName()).substring(1);

    #ifdef ficonDBG
      ficonDbgSerial.print(String(dir.fileName()).substring(1));
      ficonDbgSerial.print(F(" : "));
    #endif

    message += F("\",\"size\":");
    message += String(dir.fileSize());

    #ifdef ficonDBG
      ficonDbgSerial.println(String(dir.fileSize()));
    #endif

    message += "}"; // "}"
    entry.close();
  }
  message += "]";

  replyOKWithMsg(message, FPSTR(TEXT_JSON));
}

//=======================================================================================
//== Формирование JSON с информацией о файловой системе                                ==
//=======================================================================================
void ficonStatus() {
  if (!ficonFSOK) { // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  #ifdef ficonDBG
    ficonDbgSerial.println();
    ficonDbgSerial.println(F("== Info SPIFFS: =="));
  #endif

  FSInfo fs_info;
  String message;
  message.reserve(140); // Резервировние памяти под JSON строку
  message = F("{\"isOk\":");
  if (ficonFS.info(fs_info)) {
    message += F("\"true\",\"totalBytes\":\"");
    message += fs_info.totalBytes;

    #ifdef ficonDBG
      ficonDbgSerial.print(F("totalBytes : "));
      ficonDbgSerial.println(fs_info.totalBytes);
    #endif

    message += F("\",\"usedBytes\":\"");
    message += fs_info.usedBytes;

    #ifdef ficonDBG
      ficonDbgSerial.print(F("usedBytes : "));
      ficonDbgSerial.println(fs_info.usedBytes);
    #endif

    #ifdef ficonDBG
      ficonDbgSerial.print(F("blockSize : "));
      ficonDbgSerial.println(fs_info.blockSize);
    #endif

    #ifdef ficonDBG
      ficonDbgSerial.print(F("pageSize : "));
      ficonDbgSerial.println(fs_info.pageSize);
    #endif

    #ifdef ficonDBG
      ficonDbgSerial.print(F("maxOpenFiles : "));
      ficonDbgSerial.println(fs_info.maxOpenFiles);
    #endif

    #ifdef ficonDBG
      ficonDbgSerial.print(F("maxPathLength : "));
      ficonDbgSerial.println(fs_info.maxPathLength);
    #endif

  } else {
    message += F("\"false");
  }
  message += F("\"}");

  replyOKWithMsg(message, FPSTR(TEXT_JSON));
}

//=======================================================================================
//== Форматирование SPIFFS ESP8266                                                     ==
//=======================================================================================
void ficonFormat() {
  if (!ficonFSOK) {  // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  #ifdef ficonDBG
    ficonDbgSerial.println();
    ficonDbgSerial.print(F("Formatting the SPIFFS!"));
  #endif
  
  ficonFS.format();

  #ifdef ficonDBG
    ficonDbgSerial.println("Spiffs formatted");
  #endif
  replyOK();
}

//=======================================================================================
//== Удаление файла                                                                    ==
//=======================================================================================
void ficonDelete() {
  if (!ficonFSOK) {  // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  String delFile = ficonHttpServer.arg(0);
  if (delFile.isEmpty() || delFile == "/") {
    // Если удаляемый элемент - пустое значение
    return replyBadRequest(F("BAD PATH"));
  }

  #ifdef ficonDBG
    ficonDbgSerial.println();
    ficonDbgSerial.print(F("Deleting a file: "));
    ficonDbgSerial.print(delFile);
  #endif

  if (!ficonFS.exists(delFile)) {

    #ifdef ficonDBG
      ficonDbgSerial.println(F(" FILE NOT FOUND"));
    #endif

    // Удаляемый файл не найден
    return replyNotFound(FPSTR(FILE_NOT_FOUND));
  }

  if (ficonFS.remove(delFile)) {
    #ifdef ficonDBG
      ficonDbgSerial.println(F(" Ok"));
    #endif
    replyOK();
  } else {
    #ifdef ficonDBG
      ficonDbgSerial.println(F(" Failure"));
    #endif
    replyServerError(FPSTR(DELETE_ERROR));
  }
}

//=======================================================================================
//== Загрузка файла                                                                    ==
//=======================================================================================
void ficonUpload() {
  if (!ficonFSOK) {  // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  HTTPUpload& upload = ficonHttpServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    // Проверка наличия заглавного слэша "/"
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }

    #ifdef ficonDBG
      ficonDbgSerial.println();
      ficonDbgSerial.print(F("File upload: "));
      ficonDbgSerial.print(filename);
    #endif

    uploadFile = ficonFS.open(filename, "w"); // попытка создания нового файла
    if (!uploadFile) {

      #ifdef ficonDBG
        ficonDbgSerial.println(F(" - CREATE FAILED"));
      #endif

      return replyServerError(FPSTR(CREATE_FAILED));
    }

    #ifdef ficonDBG
      ficonDbgSerial.println();
    #endif

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
      if (bytesWritten != upload.currentSize) {

        #ifdef ficonDBG
          ficonDbgSerial.println(F(" WRITE FAILED"));
        #endif

        return replyServerError(FPSTR(WRITE_FAILED));
      }
    }

    #ifdef ficonDBG
      ficonDbgSerial.print(F("Upload: WRITE, Bytes: "));
      ficonDbgSerial.println(upload.currentSize);
    #endif

  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      replyOK();
    }
    
    #ifdef ficonDBG
      ficonDbgSerial.print(F("Upload: END, Size: "));
      ficonDbgSerial.println(upload.totalSize);
    #endif
    
  }
}

//=======================================================================================
//== Создания, переименования, перемещения файла / каталога                            ==
//=======================================================================================
void ficonFileCRM() {
  if (!ficonFSOK) {  // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }

  String dstName = ficonHttpServer.arg("dst");
  if (dstName.isEmpty()) {
    return replyBadRequest(FPSTR(DST_NAME_ARG_MISSING));
  }

  String Message = checkForUnsupportedPath(dstName);
  if (Message.length() > 0) {
    return replyServerError("INVALID FILENAME: " + Message); // ??????????????????????????????????????????????
  }

  if (dstName == "/") {
    return replyBadRequest(F("BAD PATH"));
  }

  if (ficonFS.exists(dstName)) {
    return replyBadRequest(dstName + " " + FPSTR(FILE_EXISTS));
  }

  String srcName = ficonHttpServer.arg("src");
  if (srcName.isEmpty()) {
    // Если параметр "Источник" отсутствует, то создаём элемент

    #ifdef ficonDBG
      ficonDbgSerial.println();
      ficonDbgSerial.print(F("Create item: "));
      ficonDbgSerial.println(dstName);
    #endif

    // Создаём файл
    File file = ficonFS.open(dstName, "w");
    if (file) {
      file.write((const char *)0);
      file.close();
    } else {
      // Ошибка создания файла
      return replyServerError(FPSTR(CREATE_FAILED));
    }

    // Файл успешно создан
    replyOKWithMsg(dstName);
  } else {
    // Если параметр "Источник" присутствует, то переименовываем
    if (srcName == "/") {
      return replyBadRequest(FPSTR(BAD_SRC));
    }

    if (!ficonFS.exists(srcName)) {
      // Исходный элемент не найден
      return replyBadRequest(FPSTR(SRC_FILE_NOT_FOUND));
    }

    #ifdef ficonDBG
      ficonDbgSerial.println();
      ficonDbgSerial.print(F("Renamed / moved item: "));
      ficonDbgSerial.print(srcName);
      ficonDbgSerial.print(F(" to "));
      ficonDbgSerial.println(dstName);
    #endif

    // Переименование
    if (!ficonFS.rename(srcName, dstName)) {
      return replyServerError(FPSTR(RENAME_FAILED)); // Ошибка
    }
    // Переименование выполнено успешно
    replyOKWithMsg(srcName);
  }
}

//=======================================================================================
//== Обработчик по умолчанию для всех не определенных URI                              ==
//=======================================================================================
void ficonHandleNotFound(){
  if (!ficonFSOK) {  // Если файловая система SPIFFS не проинициализирована
    return replyServerError(FPSTR(FS_INIT_ERROR));
  }
  // Строка запроса
  String path = ficonHttpServer.uri();
  // Если строка запроса заканчивается "/", то производится поиск файла index.htm
  if (path.endsWith("/")) path += F("index.htm");
  // Определение типа файла по его расширению
  String contentType = getContentType(path); // ????????????
  // Имя сжатого файла
  String pathWithGz = path + ".gz";
  // Поиск файла в SPIFFS
  if (ficonFS.exists(pathWithGz) || ficonFS.exists(path)) {
    // Проверка, если файл хранится в сжатом виде, то корректируем имя
    if (ficonFS.exists(pathWithGz))
      path += ".gz";
    // Открываем файл на чтение
    File file = ficonFS.open(path, "r");
    // Отправка файла в браузер
    size_t sent = ficonHttpServer.streamFile(file, contentType);
    // Закрываем файл
    file.close();
  } else {
    // Если файл не найден выводим сообщение об ошибке
    String message = F("File Not Found\n\n");
    message += F("URI: ");
    message += ficonHttpServer.uri();
    message += F("\nMethod: ");
    message += (ficonHttpServer.method() == HTTP_GET)?F("GET"):F("POST");
    message += F("\nArguments: ");
    message += ficonHttpServer.args();
    message += "\n";
    for (uint8_t i = 0; i < ficonHttpServer.args(); i++){
      message += " " + ficonHttpServer.argName(i) + ": " + ficonHttpServer.arg(i) + "\n";
    }
    //ficonHttpServer.send(404, "text/plain", message);
    return replyNotFound(message);
  }
}

//=======================================================================================
//== Setup ficon                                                                       ==
//== Необходимо вызвать из функции setup основной программы                            ==
//=======================================================================================
void ficonSetup() {
  #ifdef ficonInitFS
    // Инициализация библиотеки для работы с файловой системой SPIFFS
    ficonFSOK = ficonFS.begin();
    
    #ifdef ficonDBG
      ficonDbgSerial.println(ficonFSOK ? F("Filesystem initialized.") : F("Filesystem init failed!"));
    #endif
  #endif
  
  // Получение json-данных со списком файлов
  ficonHttpServer.on("/ficonFiles", HTTP_GET, ficonFiles);

  // Информация о ФС
  ficonHttpServer.on("/ficonStatus", HTTP_GET, ficonStatus);
  
  // Форматирование ФС
  ficonHttpServer.on("/ficonFormat", HTTP_GET, ficonFormat);

  // Загрузка файла
  ficonHttpServer.on("/ficonUpload",  HTTP_POST, replyOK, ficonUpload);
 
  // Удаление элемента 
  ficonHttpServer.on("/ficonDelete",  HTTP_POST, ficonDelete);
 
  // Создание / переименование / перемещение элемента
  ficonHttpServer.on("/ficonFileCRM",  HTTP_POST, ficonFileCRM);

  // Обработчик по умолчанию для всех URI, не определенных выше
  ficonHttpServer.onNotFound(ficonHandleNotFound);
}
