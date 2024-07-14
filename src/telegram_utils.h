#ifndef TELEGRAM_UTILS_H
#define TELEGRAM_UTILS_H

#define TELEGRAM_DEBUG 1

#include <UniversalTelegramBot.h>
#include "esp_camera.h"
#include "persist.h"
#include "webPages.h"
#include <ArduinoJson.h>
#include <ESP_Mail_Client.h>
static const char* TAG_TELE = "TELEGRAM";
#include "esp_log.h"


#if defined(SD_CARD_ON)
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#endif

////////////////////////////////////////////////
WiFiClientSecure botClient;
UniversalTelegramBot bot("",botClient);
camera_fb_t *fb = NULL;
size_t currentByte;

boolean bCameraInitiated=false;
boolean bTelegramBotInitiated=false;

boolean bInlineKeyboardResolution=false;
boolean bInlineKeyboardExtraOptions=false;


////////////////////////////////////////////////
/* The SMTP Session object used for Email sending */
SMTPSession smtp;
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);
////////////////////////////////////////////////
int Bot_mtbs = 1000; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done
String keyboardJson = "" ;

//boolean bSetLapseMode=false;

enum telegramAnswerCollectionMode {
  MODE_SET_NONE=0
  ,MODE_SET_lapseTime=1
  ,MODE_SET_cvIntervalSec=2
  ,MODE_SET_cvChangePercent=3
  //[:ADD INT HERE:]#4
  //[:ADD STRING HERE:]#4
} bTeleAnsMode =MODE_SET_NONE ;

////////////////////////////////////////////////
String alertTelegram(String msg,boolean messageOnly=false);
String sendCapturedImage2Telegram2(String chat_id,String messageText="",uint16_t message_id=0);
void handleNewMessages(int numNewMessages);
String formulateKeyboardJson();

/*
bool isMoreDataAvailable();
byte *getNextBuffer();
int getNextBufferLen();
bool dataAvailable = false;
//uint8_t photoNextByte();
*/
////////////////////////////////////////////////
int Counter_isMoreDataAvailable=0;
int Counter_getNextBuffer=0;
int Counter_getNextBufferLen=0;
///////////////////////////////////////////////
String alertTelegram(String msg,boolean messageOnly){
    String result="";
    #ifdef CAMERA_MODEL_M5STACK_PSRAM
      msg+=", Battery Voltage "+String(bat_get_voltage())+"mv";
    #endif
    ESP_LOGV(TAG_TELE,"AlertMessage:%s",msg);
    if (messageOnly){
      bot.sendMessage(configItems.adminChatIds, msg,"" );
    }else{
      result= sendCapturedImage2Telegram2(configItems.adminChatIds,msg);
    }
    if (configItems.alertALL && configItems.userChatIds. toDouble()>0){
      if (messageOnly){
        bot.sendMessage(configItems.userChatIds, msg,"" );
      }else{
        String result= sendCapturedImage2Telegram2(configItems.userChatIds,msg);
      }
    }
    ESP_LOGV(TAG_TELE,"%s",result);
    return(result);
}
///////////////////////////////////////////////
String formulateKeyboardJson(){

  static const char lkeyboardJson[] PROGMEM = R"(
  [
    [
       "/start \uD83D\uDCA1"
      ,"/options \uD83C\uDF33"
    ]
    ,[
       "/sendPhoto \uD83D\uDCF7"
      ,"/restartESP \uD83D\uDD50"
    ]
    ,[
      "/moreSettings \u2699"
     ,"/changeRes \uD83C\uDF3B"
    ]
    ,[ 
       "/setlapse \u23F3"
      ,"/setcvChangePercent \uFF05"
      ,"/setcvIntervalSec \u23F0"
    ]
  ]
  )";
  // [:ADD BOOLEAN HERE:]#7
  // [:ADD INT HERE:]#7
  

  ESP_LOGV(TAG_TELE,"formulateKeyboardJson: %s",lkeyboardJson);
  return(lkeyboardJson);
}
///////////////////////////////////////////////
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
    /* Print the current status */
    ESP_LOGV(TAG_TELE,"SMTP_Status.info: %s", status.info() );

    /* Print the sending result */
    if (status.success())
    {
        ESP_LOGV(TAG_TELE,"Message sent success: %d", status.completedCount());
        ESP_LOGV(TAG_TELE,"Message sent failled: %d", status.failedCount());
        struct tm dt;

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);
            //localtime_r(&result.timesstamp, &dt);

            ESP_LOGV(TAG_TELE,"Message No: %d", i + 1);
            ESP_LOGV(TAG_TELE,"Status    : %s", result.completed ? "success" : "failed");
            //Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
            ESP_LOGV(TAG_TELE,"Recipient : %s", result.recipients);
            ESP_LOGV(TAG_TELE,"Subject   : %s", result.subject);
        }
        ESP_LOGV(TAG_TELE,"----------------\n");
    }
}

///////////////////////////////////////////////
String formulateResolutionInlineKeyBoard(){
  String lkeyboardJson = "[";
  String sep="";
  /*
  int maxRes=0;
  if(psramFound()){
    maxRes = FRAMESIZE_UXGA;
  } else {
    maxRes = FRAMESIZE_SVGA;
  }
  maxRes = FRAMESIZE_UXGA;
  */
  ESP_LOGV(TAG_TELE,"formulateResolutionInlineKeyBoard:maxRes: %d",maxRes);
  for (int i=0;i<=maxRes;i++){
    String checkMark="";
    if (configItems.frameSize == i){
      checkMark="\u2705";
    }
    lkeyboardJson +=  sep+"[{";
    lkeyboardJson += "\"text\":\""+
            checkMark+
            String(resolutions[i][0])+":"+
            String(resolutions[i][1])+
            "\",\"callback_data\":\"/"+String(resolutions[i][0])+"\"";
    lkeyboardJson +=  "}]";
    sep=",";
  }
  lkeyboardJson += "]";
  ESP_LOGV(TAG_TELE,"formulateResolutionInlineKeyBoard: %s",lkeyboardJson);
  return(lkeyboardJson);
}
///////////////////////////////////////////////
String formulateOptionsInlineKeyBoard(){

  // get more unicodes from here https://apps.timwhitlock.info/emoji/tables/unicode
  String keyboardJson = "["; // start Json
  #if defined(IS_THERE_A_FLASH)
    keyboardJson += "[{ \"text\" : \"The Flash is ";
    keyboardJson += (configItems.useFlash?"ON\u2705":"OFF\u274C");
    keyboardJson += "\", \"callback_data\" : \"/useFlash\" }],";
  #endif

  #if defined(I2C_DISPLAY_ADDR)
    keyboardJson += "[{ \"text\" : \"Screen is ";
    keyboardJson += (configItems.screenOn?"ON\u2705":"OFF\u274C");
    keyboardJson += "\", \"callback_data\" : \"/screenOn\" }],";

    keyboardJson += "[{ \"text\" : \"screen Flip is ";
    keyboardJson += (configItems.screenFlip?"ON\u2705":"OFF\u274C");
    keyboardJson += "\", \"callback_data\" : \"/screenFlip\" }],";
  #endif

  #if defined(PIR_PIN)
    keyboardJson += "[{ \"text\" : \"PIR MotionDetect is:";
    keyboardJson += (configItems.motDetectOn?"ON\u2705":"OFF\u274C");
    keyboardJson += "\", \"callback_data\" : \"/motDetectOn\" }],";
  #endif
  keyboardJson += R"([{ "text" : "CV MotionDetect is:)";
  keyboardJson += (configItems.motionDetectVC?"ON\u2705":"OFF\u274C");
  keyboardJson += R"(", "callback_data" : "/motionDetectVC" }],)";
  if (psramFound()){
    // for face detection
  }

  // [:ADD BOOLEAN HERE:]#1

  keyboardJson += "[{ \"text\" : \"Camera Mirror is:";
  keyboardJson += (configItems.hMirror?"ON\u2705":"OFF\u274C");
  keyboardJson += "\", \"callback_data\" : \"/hMirror\" }],";

  keyboardJson += "[{ \"text\" : \"Camera Flip is:";
  keyboardJson += (configItems.vFlip?"ON\u2705":"OFF\u274C");
  keyboardJson += "\", \"callback_data\" : \"/vFlip\" }],";

  keyboardJson += R"([{ "text" : "Alert by Email. )";
  keyboardJson += (configItems.sendEmail?"ON\u2705":"OFF\u274C");
  keyboardJson += R"(", "callback_data" : "/sendEmail" }],)";

  keyboardJson += R"([{ "text" : "Alert all:)";
  keyboardJson += (configItems.alertALL?"ON\u2705":"OFF\u274C");
  keyboardJson += R"(", "callback_data" : "/alertALL" }],)";

  #if defined(SD_CARD_ON)
    keyboardJson += R"([{ "text" : "Save to SD:)";
    keyboardJson += (configItems.saveToSD?"ON\u2705":"OFF\u274C");
    keyboardJson += R"(", "callback_data" : "/saveToSD" }],)";
  #endif

  keyboardJson += R"([{ "text" : "Enable deep sleep:)";
  keyboardJson += (configItems.useDeepSleep?"ON\u2705":"OFF\u274C");
  keyboardJson += R"(", "callback_data" : "/useDeepSleep" }],)";

  #if defined(BUZZER_PIN)
    keyboardJson += R"([{ "text" : "Buzz on MotionDetect:)";
    keyboardJson += (configItems.useBuzzer?"ON\u2705":"OFF\u274C");
    keyboardJson += R"(", "callback_data" : "/useBuzzer" }],)";
  #endif

  keyboardJson += "[{ \"text\" : \"Web Capture is:";
  keyboardJson += (configItems.webCaptureOn?"ON\u2705":"OFF\u274C");
  keyboardJson += "\", \"callback_data\" : \"/webCaptureOn\" }],";

  keyboardJson += "[{ \"text\" : \"OTA is:";
  keyboardJson += (acConfig.ota==AC_OTA_BUILTIN?"ON\u2705":"OFF\u274C");
  keyboardJson += "\", \"callback_data\" : \"/OTAOn\" }]";

  keyboardJson += "]";

  ESP_LOGV(TAG_TELE,"formulateOptionsInlineKeyBoard: %s", keyboardJson);
  return(keyboardJson);
}

////////////////////////////////////
void handleNewMessages(int numNewMessages) {
  ESP_LOGV(TAG_TELE,"handleNewMessages:BEGIN");
  ESP_LOGV(TAG_TELE,"numNewMessages:%d",numNewMessages);
  boolean bPrintOptions=true;
  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    uint16_t message_id = bot.messages[i].message_id;
    ESP_LOGV(TAG_TELE,"ChatID: %s",chat_id);
    if (chat_id.equals(configItems.adminChatIds) || chat_id.equals(configItems.userChatIds)) {
      // If the type is a "callback_query", a inline keyboard button was pressed
      if (bot.messages[i].type ==  "callback_query") {
        String text = bot.messages[i].text;
        bPrintOptions=false;
        ESP_LOGV(TAG_TELE,"Call back button pressed with text: %s",text);
        //////////////////////
        //if resolution change
        if ( bInlineKeyboardResolution ){
          if (matchResolutionText(text)){
            //framesize_t https://github.com/espressif/esp32-camera/blob/master/driver/include/sensor.h
            int result=matchResolutionText(text);
            result=(result<0?0:result);
            configItems.frameSize=(framesize_t) result;
          }
          bot.sendMessageWithInlineKeyboard(chat_id,
           "Select the resolution", 
           "Markdown", 
           formulateResolutionInlineKeyBoard(),
           message_id
           );
        }
        if ( bInlineKeyboardExtraOptions ){          
          if (text == "/hMirror") {
            configItems.hMirror = !configItems.hMirror;
          }else if (text == "/vFlip") {
            configItems.vFlip = !configItems.vFlip;
          }else if (text == "/sendEmail") {
            configItems.sendEmail = !configItems.sendEmail;
          }else if (text == "/motionDetectVC") {
            configItems.motionDetectVC = !configItems.motionDetectVC;
          }else if (text == "/alertALL") {
            configItems.alertALL = !configItems.alertALL;
          }else if (text == "/saveToSD") {
            configItems.saveToSD = !configItems.saveToSD;
          }else if (text == "/useDeepSleep") {
            configItems.useDeepSleep = !configItems.useDeepSleep;
          }else if (text == "/useBuzzer") {
            configItems.useBuzzer = !configItems.useBuzzer;
          }else if (text == "/useFlash") {
            configItems.useFlash = !configItems.useFlash;
          }else if (text == "/screenFlip") {
            configItems.screenFlip = !configItems.screenFlip;
          }else if (text == "/screenOn") {
            configItems.screenOn = !configItems.screenOn;
          }else if (text == "/motDetectOn") {
            configItems.motDetectOn = !configItems.motDetectOn;
          }else if (text == "/webCaptureOn") {
            configItems.webCaptureOn = !configItems.webCaptureOn;
            // [:ADD BOOLEAN HERE:]#2
          }else if (text == "/OTAOn") {
            if (acConfig.ota==AC_OTA_BUILTIN )
              acConfig.ota=AC_OTA_EXTRA;
            else
              acConfig.ota=AC_OTA_BUILTIN;
            Portal.config(acConfig);
          }
          bot.sendMessageWithInlineKeyboard(chat_id,
            "Change settings:",
            "html",
            formulateOptionsInlineKeyBoard(),
            message_id
            );
        }
        //////////////////////
        ESP_LOGV(TAG_TELE,"saveConfiguration(configItems);");
        boolean bDirty=saveConfiguration(&configItems);
        ESP_LOGV(TAG_TELE,"applyConfigItem(&configItems)#1");
        applyConfigItem(&configItems);
      } else {
        String text = String(bot.messages[i].text);
        String from_name = bot.messages[i].from_name;
        if (from_name == "")
          from_name = "Guest";
        ESP_LOGV(TAG_TELE,"TEXT: %s",text);
        //[:ADD INT HERE:]#5
        //[:ADD STRING HERE:]#5
        if (bTeleAnsMode==MODE_SET_lapseTime) {
          bTeleAnsMode=MODE_SET_NONE;
          configItems.lapseTime=(int) text.toInt(); }
        if (bTeleAnsMode==MODE_SET_cvChangePercent) {
          bTeleAnsMode=MODE_SET_NONE;
          configItems.cvChangePercent=(int) text.toInt(); }
        if (bTeleAnsMode==MODE_SET_cvIntervalSec) {
          bTeleAnsMode=MODE_SET_NONE;
          configItems.cvIntervalSec=(int) text.toInt(); }
        ////////////////////////////////////////////////////
        if (text.startsWith("/sendPhoto") || text.startsWith("/sendphoto")) {
          ESP_LOGV(TAG_TELE,"handleNewMessages/sendPhoto:BEGIN");
          if(bot.messages[i].type == "channel_post") {
            bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].chat_title + " " + bot.messages[i].text, "");
          } else {
            String msg="Photo Request";
            #ifdef CAMERA_MODEL_M5STACK_PSRAM
              msg+=", Battery Voltage "+String(bat_get_voltage())+"mv";
            #endif
            String result= sendCapturedImage2Telegram2(chat_id,msg,message_id);
            ESP_LOGV(TAG_TELE,"result: %s",result);
          }
          ESP_LOGV(TAG_TELE,"handleNewMessages/sendPhoto:END");
          bPrintOptions=false;
        }else if(text.startsWith("/options")) {
          bPrintOptions=true;
          if(bot.sendMessageWithReplyKeyboard(chat_id, "", "Markdown", formulateKeyboardJson(), true) )
            ESP_LOGV(TAG_TELE,"bot.sendMessageWithReplyKeyboard:TRUE");
          else
            ESP_LOGV(TAG_TELE,"bot.sendMessageWithReplyKeyboard:FALSE");
        }else if(text.startsWith("/changeRes") ){
          bPrintOptions=false;
          bInlineKeyboardExtraOptions=false;
          bInlineKeyboardResolution=true;
          bot.sendMessageWithInlineKeyboard(chat_id, "Select the resolution", "", formulateResolutionInlineKeyBoard());
        }
        //[:ADD INT HERE:]#6
        //[:ADD STRING HERE:]#6
        else if(text.startsWith("/setlapse") ){
          configItems.lapseTime=0;
          bot.sendMessage(chat_id, "Please insert Lapse Time in minutes:", "");
          bTeleAnsMode=MODE_SET_lapseTime;
          bPrintOptions=false;
        }
        else if(text.startsWith( "/setcvChangePercent") ) {
          configItems.cvChangePercent=0;
          bot.sendMessage(chat_id, "Please insert CV change percentage:", "");
          bTeleAnsMode=MODE_SET_cvChangePercent;
          bPrintOptions=false;
        }
        else if(text.startsWith("/setcvIntervalSec") ){
          configItems.cvIntervalSec=0;
          bot.sendMessage(chat_id, "Please insert time in ms between two CV frames:", "");
          bTeleAnsMode=MODE_SET_cvIntervalSec;
          bPrintOptions=false;
        } else if(text.startsWith("/moreSettings") ){
          configItems.useDeepSleep=false;
          bPrintOptions=false;
          bInlineKeyboardResolution=false;
          bInlineKeyboardExtraOptions=true;
          bot.sendMessageWithInlineKeyboard(chat_id, "Change settings:", "html", formulateOptionsInlineKeyBoard());
        }else if( text.startsWith("/restartESP") ) {
          numNewMessages = bot.getUpdates((bot.last_message_received) + 1);
          ESP.restart();
        }else if ( text.startsWith("/start") ){
          String welcome = "```\n";
          welcome += "*Command*    |*Description*\n";
          welcome += "-----|-----\n";
          welcome += "/start       | sends this message\n";
          welcome += "/options     | returns the reply keyboard\n";
          welcome += "/setlapse    | Sets the periodical sending of photo in min (0 is disable)\n";
          // [:ADD BOOLEAN HERE:]#8
          // [:ADD INT HERE:]#8
          welcome += "/setcvChangePercent    | Sets the CV Motion Detect Percent change in number of DarkPixels:\n";
          welcome += "/setcvIntervalSec    | Sets the CV Motion Detect interval between each 2 consecutive photo capture:\n";
          
          welcome += "/sendPhoto   | Send a Photo from the camera\n";
          welcome += "/changeRes   | Change the resoltion of the camera\n";
          welcome += "/moreSettings| Access more settings\n";
          welcome += "\t hMirror      | Camera horizontal MIRROR\n";
          welcome += "\t vFlip        | Camera vertical FLIP\n";
          welcome += "\t motionDetectVC | MotionDetect by Vision not PIR -no DeepSleep\n";
          welcome += "\t alertALL     | MotionDetect/timelapse to all chatIds not just Admin\n";
          welcome += "\t saveToSD     | Save Photos to SD also\n";
          welcome += "\t sendEmail    | Alert by Email also\n";
          welcome += "\t useDeepSleep | goto to deep sleep on MotionDetct and timeLapse\n";
          welcome += "\t useBuzzer    | trigger buzzer on motion detect\n";
          welcome += "\t webCaptureOn | enables and disbales the /capture.jpg url\n";
          welcome += "\t OTAOn        | enables OTA through local Wifi \n";
          // [:ADD BOOLEAN HERE:]#3
  #if defined(IS_THERE_A_FLASH)
          welcome += "\t useFlash    | Flash is enabled\n";
  #endif
  #if defined(I2C_DISPLAY_ADDR)
          welcome += "\t screenFlip  | Screen flip\n";
          welcome += "\t scrrenOn    | Screen enabled\n";
  #endif
  #if defined(PIR_PIN)
          welcome += "\t motDetectOn | Motion detection enabled\n";
  #endif
          welcome += "```";
          ESP_LOGV(TAG_TELE,"welcome message:%s",welcome);
          bot.sendMessage(chat_id, welcome, "Markdown");
          ////////////////////////////////////////
          bot.sendMessageWithReplyKeyboard(chat_id, "Choose from one of the options below:", "", formulateKeyboardJson(), true);
        }
      }
      ///////////////////////////////////
      ESP_LOGV(TAG_TELE,"saveConfiguration(configItems);");
      boolean bDirty=saveConfiguration(&configItems);
      ESP_LOGV(TAG_TELE,"applyConfigItem(&configItems)#2");
      applyConfigItem(&configItems);
      if(bPrintOptions){
        ESP_LOGV(TAG_TELE,"printConfiguration(&configItems);");
        ESP_LOGV(TAG_TELE,"%s",printConfiguration(&configItems,"","\n","|"));
        bot.sendMessage(chat_id, printConfiguration(&configItems,"","\n","|"), "HTML");
        bPrintOptions=false;
        }
    } else {
      bot.sendMessage(chat_id, 
      " \'" + bot.messages[i].text + "\' from "+ chat_id
      , "");
    }
  }
  ESP_LOGV(TAG_TELE,"handleNewMessages:END");
}

///////////////////////////////////////////////
void sendEmailPhoto(camera_fb_t * fb ,String espMessage="Teleview Photo"){
  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(1);
  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  //boolean sMTPTLS;

  /* Set the session config */
  session.server.host_name = configItems.sMTPServer.c_str();
  session.server.port = configItems.sMTPPort;
  session.login.email = configItems.sMTPUsername.c_str();
  session.login.password = configItems.sMTPPassword.c_str();
  //session.login.user_domain = "hotmail.com";

  /* Declare the message class */
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

  /* Set the message headers */
  message.sender.name = "ESP Mail";
  message.sender.email = configItems.sMTPUsername.c_str();

  message.subject = espMessage.c_str();
  message.addRecipient("Admin", configItems.adminEmail.c_str());
  message.addRecipient("User", configItems.userEmail.c_str());

  int fb_width=fb->width;
  int fb_height=fb->height;
  String msg= R"(
      <span style="color:#ff0000;">
          {{espMessage}}
      </span>
      <br/><br/>
      <img width="{{WIDTH}}" height="{{HEIGHT}}" src="firebase_logo.jpeg"  >
      )";
  msg.replace("{{espMessage}}",espMessage);
  msg.replace("{{WIDTH}}",String(fb_width) );
  msg.replace("{{HEIGHT}}",String(fb_height) );
  message.html.content =msg.c_str();

  /** The HTML text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
  */
  message.html.charSet = "utf-8";

  /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
  */
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.text.content = "This message contains 1 inline image.\r\nThe inline images were not shown in the plain text message.";
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
  */
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

  /** The Delivery Status Notifications e.g.
   * esp_mail_smtp_notify_never
   * esp_mail_smtp_notify_success
   * esp_mail_smtp_notify_failure
   * esp_mail_smtp_notify_delay
   * The default value is esp_mail_smtp_notify_never
  */
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  //+configItems.sMTPUsername
  message.addHeader( "Message-ID: user@domain.org");

  /* The attachment data item */
  SMTP_Attachment att;

  /** Set the inline image info e.g. 
   * file name, MIME type, BLOB data, BLOB data size,
   * transfer encoding (should be base64 for inline image)
  */
  att.descr.filename = "firebase_logo.jpeg";
  att.descr.mime = "image/jpeg";
  att.blob.data = fb->buf;
  att.blob.size = fb->len;
  //att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  att.descr.content_encoding = Content_Transfer_Encoding::enc_binary;

  // Add attachment to the message
  //message.addAttachment(att);

  /* Add inline image to the message */
  message.addInlineImage(att);

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending the Email and close the session */
  if (!MailClient.sendMail(&smtp, &message, true))
    ESP_LOGE(TAG_TELE,"Error sending Email: %s " , smtp.errorReason());
}

///////////////////////////////////////////////
String sendCapturedImage2Telegram2(String chat_id,String messageText ,uint16_t message_id) {
  const char* myDomain = "api.telegram.org";
  String getAll="", getBody = "";
  StaticJsonDocument<2048> doc;
  String result="success";
  camera_fb_t * fb = NULL;

#if defined(FLASH_LAMP_PIN)
  if (configItems.useFlash){
    ESP_LOGV(TAG_TELE,"Switching Flash-lamp ON");
    digitalWrite(FLASH_LAMP_PIN, HIGH);
    delay(250);
    ESP_LOGV(TAG_TELE,"The Flash-lamp is ON");
  }
#endif

#if defined(USE_OLED_AS_FLASH)
  if (configItems.useFlash){
    ESP_LOGV(TAG_TELE,"Switching Flash-OLED ON");
    display_AllWhite();
    delay(250);
    ESP_LOGV(TAG_TELE,"Flash-OLED is ON");
  }
#endif

  ESP_LOGV(TAG_TELE,"Capture Photo");
  for (int i=0; i<2; i++) {
    fb = esp_camera_fb_get();
    delay(1500);
    esp_camera_fb_return(fb);
  }
  fb = esp_camera_fb_get();
  if(!fb) {
    ESP_LOGE(TAG_TELE,"Camera capture failed");
    delay(1000);

    #if defined(FLASH_LAMP_PIN)
    if (configItems.useFlash){
      delay(10);
     digitalWrite(FLASH_LAMP_PIN, LOW); 
     ESP_LOGV(TAG_TELE,"Flash-lamp OFF");
    }
    #endif
    ESP.restart();
  }
#if defined(FLASH_LAMP_PIN)
  if (configItems.useFlash){
    delay(10);
    digitalWrite(FLASH_LAMP_PIN, LOW); 
    ESP_LOGV(TAG_TELE,"Flash-lamp OFF");
  }
#endif
  delay(2000);
  int fb_width=fb->width;
  int fb_height=fb->height;
  ESP_LOGV(TAG_TELE,"sendChatAction#1");
  bot.sendChatAction(chat_id, "upload_photo");
  // reset the client connection
  if (botClient.connected()) {
    #ifdef TELEGRAM_DEBUG
        ESP_LOGV(TAG_TELE,"Closing client");
    #endif
    botClient.stop();
  }
  // Connect with api.telegram.org if not already connected
  if (!botClient.connected()) {
    #ifdef TELEGRAM_DEBUG
        ESP_LOGV(TAG_TELE,"[BOT Client]Connecting to server" );
    #endif
    if (!botClient.connect(TELEGRAM_HOST, TELEGRAM_SSL_PORT)) {
      #ifdef TELEGRAM_DEBUG
        ESP_LOGE(TAG_TELE,"[BOT Client]Conection error" );
      #endif
    }
  }

  String head ="";
  head += "--ef2ac69f9149220e889abc22b81d1401\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id +"\r\n";
  head += "--ef2ac69f9149220e889abc22b81d1401\r\nContent-Disposition: form-data; name=\"caption\"; \r\n\r\n" +
            String(fb_width)+"x"+String(fb_height)+", "+String(fb->len) +" B"+
            ","+messageText+
            "\r\n";
  if (message_id>0)
    head += "--ef2ac69f9149220e889abc22b81d1401\r\nContent-Disposition: form-data; name=\"reply_to_message_id\"; \r\n\r\n" + String(message_id) +"\r\n";
  head += "--ef2ac69f9149220e889abc22b81d1401\r\nContent-Disposition: form-data; name=\"parse_mode\"; \r\n\r\nMarkdown\r\n";
  head += "--ef2ac69f9149220e889abc22b81d1401\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\n";
  //"Content-Type: image/jpeg\r\n\r\n"+

  String tail = "\r\n--ef2ac69f9149220e889abc22b81d1401--\r\n";

  uint32_t imageLen = fb->len;
  uint32_t extraLen = head.length() + tail.length();
  uint32_t totalLen = imageLen + extraLen;

  botClient.println("POST /bot"+configItems.botTTelegram+"/sendPhoto HTTP/1.1");
  botClient.println("Host: " + String(myDomain));
  botClient.println("Connection: keep-alive");
  botClient.println("Content-Length: " + String(totalLen));
  botClient.println("Content-Type: multipart/form-data; boundary=ef2ac69f9149220e889abc22b81d1401");
  botClient.println();
  botClient.print(head);
  botClient.println();

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  size_t sentB =0;
  Serial.print("/");
  for (size_t n=0;n<fbLen;n=n+1024) {
    if (n+1024<fbLen) {
      Serial.print("_");
      botClient.write(fbBuf, 1024);
      fbBuf += 1024;
      sentB += 1024;
    }
    else if (fbLen%1024>0) {
      Serial.print("+");
      size_t remainder = fbLen%1024;
      botClient.write(fbBuf, remainder);
      sentB += remainder;
    }
    if(botClient.connected())
      Serial.print("*");
    else
      Serial.print("X");
  }
  Serial.println("/");
  botClient.print(tail);

  ESP_LOGV(TAG_TELE, "sendCapturedImage2Telegram2:sentB:%d , w%d X h%d",
    sentB,
    fb_width,
    fb_height
    );

  int waitTime = 10000;   // timeout 10 seconds
  long startTime = millis();
  boolean state = false;

  while ((startTime + waitTime) > millis())
  {
    Serial.print(".");
    //delay(100);
    while (botClient.available())
    {
        char c = botClient.read();
        if (state==true) {
          getBody += String(c);
        }
        /*
        else{
          botClient.write(fbBufX,oneByte);
        }
        */
        if (c == '\n')
        {
          if (getAll.length()==0) state=true;
          getAll = "";
        }
        else if (c != '\r')
          getAll += String(c);
        startTime = millis();
     }
     if (getBody.length()>0) break;
  }
  ESP_LOGV(TAG_TELE,"sendCapturedImage2Telegram2:getBody: %s",getBody);
  /*
  // Deserialize the JSON document
  // sample:
  // {"ok":false,"error_code":400,"description":"Bad Request: IMAGE_PROCESS_FAILED"}
  // {"ok":true,"result":{"message_id":3914,"from":{"id":1748863501,"is_bot":true,"first_name":"ESP32_CAM_04_bot","username":"ESP32_CAM_04_bot"},"chat":{"id":591479121,"first_name":"A","last_name":"Maklad","username":"asmaklad","type":"private"},"date":1620390975,"photo":[{"file_id":"AgACAgQAAxkDAAIPSmCVND8eYR6M8XHIZykZp_CHHBI-AAKhuDEbWquoUIa2-YNz89PNEjUyLl0AAwEAAwIAA20AAx4iAQABHwQ","file_unique_id":"AQADEjUyLl0AAx4iAQAB","file_size":9221,"width":320,"height":240},{"file_id":"AgACAgQAAxkDAAIPSmCVND8eYR6M8XHIZykZp_CHHBI-AAKhuDEbWquoUIa2-YNz89PNEjUyLl0AAwEAAwIAA3gAAx8iAQABHwQ","file_unique_id":"AQADEjUyLl0AAx8iAQAB","file_size":26810,"width":800,"height":600}],"caption":"800x600, 26810 B"}}
  */
  if (getBody.equals("")){
    result="Empty Response, Sending Failed.";
  }else{
    DeserializationError error = deserializeJson(doc, getBody);
    // Test if parsing succeeds.
    if (error) {
      ESP_LOGE(TAG_TELE,"sendCapturedImage2Telegram2:deserializeJson() failed: ");
      ESP_LOGE(TAG_TELE,"%s",error.c_str());
      result="Can't parse response.";
    }
    boolean responseOK=doc["ok"];
    if (!responseOK){
      result=doc["description"].as<String>();
    }
  }
  PICTURES_COUNT++;
  botClient.stop();
  // send Email
  if (configItems.sendEmail){
    if (configItems.sMTPPort>0 &&
        configItems.sMTPPassword.length() >0 &&
        configItems.sMTPUsername.length() >0 &&
        configItems.sMTPServer.length() >0 &&
        configItems.adminEmail.length() >0
        )
    {
      sendEmailPhoto(fb,messageText);
    }
  }
  // saving to SD card.
#if defined(SD_CARD_ON)
  if (configItems.saveToSD) {
    ESP_LOGV(TAG_TELE,"Saving to SD Card.");
    if(!SD_MMC.begin()){
      ESP_LOGV(TAG_TELE,"SD Card Mount Failed");
      result="SD Card Mount Failed";
    }else{
      uint8_t cardType = SD_MMC.cardType();
      if(cardType == CARD_NONE){
        ESP_LOGV(TAG_TELE,"No SD Card attached");
        result="No SD Card attached";
      }else{
        struct tm *tm;
        time_t  t;
        char    dateTime[100];
        t = time(NULL);
        tm = localtime(&t);
        sprintf(dateTime, "-%04d%02d%02d_%02d%02d%02d",
          tm->tm_year + 1900, tm->tm_mon+1 , tm->tm_mday, 
          tm->tm_hour, tm->tm_min, tm->tm_sec);
        // Path where new picture will be saved in SD Card
        String path = "/picture" + String(dateTime) + "_" + messageText+".jpg";
        fs::FS &fs = SD_MMC;
        ESP_LOGV(TAG_TELE,"Picture file name: %s\n", path.c_str());
        File file = fs.open(path.c_str(), FILE_WRITE);
        if(!file){
          ESP_LOGV(TAG_TELE,"Failed to open file in writing mode");
        } else {
          file.write(fb->buf, fb->len); // payload (image), payload length
          ESP_LOGV(TAG_TELE,
                    "Saved file to path: %s, Card Size: %llu, Total KBytes: %llu, Used KBytes: %llu"
                    , path.c_str()
                    , SD_MMC.cardSize()
                    ,SD_MMC.totalBytes()/1024
                    ,SD_MMC.usedBytes()/1024
          );
        }
        file.close();
      }
    }
#if defined(FLASH_LAMP_PIN)
    if (configItems.useFlash){
      delay(10);
      digitalWrite(FLASH_LAMP_PIN, LOW); 
      ESP_LOGV(TAG_TELE,"Flash-lamp OFF");
    }
#endif
  }
#endif
  esp_camera_fb_return(fb);

#if defined(USE_OLED_AS_FLASH)
  display_Clear();
#endif
  if (!result.equals("success")){
    bot.sendMessage(chat_id, String("Photo sent:"+String(fb_width)+"x"+String(fb_height))+","+String(fbLen/1024)+" KB:"+result,"" );
  }
  return(result);
}


#endif //TELEGRAM_UTILS_H