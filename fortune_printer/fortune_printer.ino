// Fortune Printer - Stage 3b
// ESP32-C6: always-on WiFi (own access point + saved home network), fortunes
// stored in LittleFS (upload, edit, download, delete, choose which prints),
// optional admin password, button prints a random fortune on a BLE cat
// printer (see cat_printer.h, unchanged).
//
// Button: tap = print a fortune, hold 8 seconds = forget the saved WiFi and
// the admin password.
// Web:    http://fortune.local (home network) or http://192.168.4.1 (its own
//         "FortunePrinter" WiFi).
//
// Libraries: everything ships with the ESP32 core, plus the two that
// cat_printer.h needs (NimBLE-Arduino 2.x, Adafruit GFX).
// Partition scheme: keep "Huge APP (3MB No OTA/1MB SPIFFS)".

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <vector>
#include "cat_printer.h"

// ---- Settings ----
const int   BUTTON_PIN = 9;               // BOOT button on most ESP32-C6 dev boards
const char* AP_SSID    = "FortunePrinter";
const char* HOST_NAME  = "fortune";       // -> http://fortune.local
#define WIFI_TIMEOUT_MS  10000
#define CONFIG_PATH      "/config.txt"
#define FORTUNE_DIR      "/fortunes"

WebServer server(80);

String cfgSsid;
String cfgPass;
String cfgActive = "*";      // "*" = mix every fortune file, or one file name
String scannedOptions;       // <option> list of networks seen at boot
bool fsOk = false;
bool printRequested = false;
bool pendingRestart = false;
unsigned long restartAt = 0;

String cfgAdmin;                // optional admin password (user name is "admin")
bool   cfgLucky = false;        // add lucky numbers to each printed fortune
bool   cfgTitle = true;         // print the banner (on by default)
String pickedHeading;           // heading of the file the last fortune came from ("" = FORTUNE)
int    pickedLucky = -1;        // that file's lucky numbers choice: -1 follow the web setting, 0 off, 1 on
#define MAX_FILE_BYTES  100000  // largest file you can upload
#define EDIT_MAX_BYTES  30000   // largest file the browser editor will open
File   uploadFile;
String uploadName;
size_t uploadBytes  = 0;
size_t uploadFree   = 0;
bool   uploadFailed = false;
bool   uploadSeen   = false;

// Starter fortunes, written to /fortunes/general.txt on first boot
const char DEFAULT_FORTUNES[] = R"FORTUNES(
A beautiful, smart, and loving person will be coming into your life.
A dubious friend may be an enemy in camouflage.
A faithful friend is a strong defense.
A feather in the hand is better than a bird in the air.
A fresh start will put you on your way.
A friend asks only for your time not your money.
A friend is a present you give yourself.
A funny coincidence will make your day.
A gambler not only will lose what he has, but also will lose what he doesn’t have.
A golden egg of opportunity falls into your lap this month.
A good friendship is often more important than a passionate romance.
A good time to finish up old tasks.
A hunch is creativity trying to tell you something.
A lifetime friend shall soon be made.
A lifetime of happiness lies ahead of you.
A light heart carries you through all the hard times.
A new outlook brightens your image and brings new friends.
A new perspective will come with the new year.
A person is never to (sic) old to learn.
A person of words and not deeds is like a garden full of weeds.
A pleasant surprise is waiting for you.
A short pencil is usually better than a long memory any day.
A small donation is call for. It’s the right thing to do.
A smile is your personal welcome mat.
A smooth long journey! Great expectations.
A soft voice may be awfully persuasive.
A truly rich life contains love and art in abundance.
Accept something that you cannot change, and you will feel better.
Adventure can be real happiness.
Advice is like kissing. It costs nothing and is a pleasant thing to do.
Advice, when most needed, is least heeded.
All the effort you are making will ultimately pay off.
All the troubles you have will pass away very quickly.
All will go well with your new project.
All your hard work will soon pay off.
Allow compassion to guide your decisions.
An acquaintance of the past will affect you in the near future.
An agreeable romance might begin to take on the appearance.
An important person will offer you support.
An inch of time is an inch of gold.
Any day above ground is a good day.
Any decision you have to make tomorrow is a good decision.
At the touch of love, everyone becomes a poet.
Be careful or you could fall for some tricks today.
Beauty in its various forms appeals to you.
Because you demand more from yourself, others respect you deeply.
Believe in yourself and others will too.
Believe it can be done.
Better ask twice than lose yourself once.
Bide your time, for success is near.
Carve your name on your heart and not on marble.
Chance favors those in motion.
Change is happening in your life, so go with the flow!
Competence like yours is underrated.
Congratulations! You are on your way.
Could I get some directions to your heart?
Courtesy begins in the home.
Courtesy is contagious.
Curiosity kills boredom. Nothing can kill curiosity.
Dedicate yourself with a calm mind to the task at hand.
Depart not from the path which fate has you assigned.
Determination is what you need now.
Diligence and modesty can raise your social status.
Disbelief destroys the magic.
Distance yourself from the vain.
Do not be intimidated by the eloquence of others.
Do not demand for someone’s soul if you already got his heart.
Do not let ambitions overshadow small success.
Do not make extra work for yourself.
Do not underestimate yourself. Human beings have unlimited potentials.
Do you know that the busiest person has the largest amount of time?
Don’t be afraid to admit when you need a break.
Don’t be discouraged, because every wrong attempt discarded is another step forward.
Don’t confuse recklessness with confidence.
Don’t expect romantic attachments to be strictly logical or rational.
Don’t just spend time. Invest it.
Don’t just think, act!
Don’t let friends impose on you, work calmly and silently.
Don’t let the past and useless detail choke your existence.
Don’t let your limitations overshadow your talents.
Don’t worry; prosperity will knock on your door soon.
Each day, compel yourself to do something you would rather not do.
Education is the ability to meet life’s situations.
Embrace this love relationship you have!
Emulate what you admire in your parents.
Emulate what you respect in your friends.
Every flower blooms in its own sweet time.
Every wise man started out by asking many questions.
Everyday in your life is a special occasion.
Everywhere you choose to go, friendly faces will greet you.
Expect much of yourself and little of others.
Failure is the chance to do better next time.
Failure is the path of lease persistence.
Fear and desire – two sides of the same coin.
Fearless courage is the foundation of victory.
Feeding a cow with roses does not get extra appreciation.
First think of what you want to do; then do what you have to do.
Follow the middle path. Neither extreme will make you happy.
For hate is never conquered by hate. Hate is conquered by love.
For the things we have to learn before we can do them, we learn by doing them.
Fortune Not Found: Abort, Retry, Ignore?
From listening comes wisdom and from speaking repentance.
From now on your kindness will lead you to success.
Get your mind set – confidence will lead you on.
Get your mind set…confidence will lead you on.
Go for the gold today! You’ll be the champion of whatever.
Go take a rest; you deserve it.
Good news will be brought to you by mail.
Good news will come to you by mail.
Good to begin well, better to end well.
Happiness begins with facing life with a smile and a wink.
Happiness will bring you good luck.
Happy life is just in front of you.
Hard words break no bones, fine words butter no parsnips.
Have a beautiful day.
He who expects no gratitude shall never be disappointed.
He who knows he has enough is rich.
He who knows others is wise. He who knows himself is enlightened.
Help! I’m being held prisoner in a chinese bakery!
How many of you believe in psycho-kinesis? Raise my hand.
How you look depends on where you go.
I learn by going where I have to go.
If a true sense of value is to be yours it must come through service.
If certainty were truth, we would never be wrong.
If you continually give, you will continually have.
If you look in the right places, you can find some good offerings.
If you think you can do a thing or think you can’t do a thing, you’re right.
If you wish to see the best in others, show the best of yourself.
If your desires are not extravagant, they will be granted.
If your desires are not to extravagant they will be granted.
If you’re feeling down, try throwing yourself into your work.
Imagination rules the world.
In order to take, one must first give.
In the end all things will be known.
In this world of contradiction, It’s better to be merry than wise.
It could be better, but its good enough.
It is better to be an optimist and proven a fool than to be a pessimist and be proven right.
It is better to deal with problems before they arise.
It is honorable to stand up for what is right, however unpopular it seems.
It is worth reviewing some old lessons.
It takes courage to admit fault.
It’s not the amount of time you devote, but what you devote to the time that counts.
It’s time to get moving. Your spirits will lift accordingly.
Keep your face to the sunshine and you will never see shadows.
Kindness is the currency of the soul.
Let the world be filled with tranquility and goodwill.
Like the river flow into the sea. Something are just meant to be.
Listen not to vain words of empty tongue.
Listen to everyone. Ideas come from everywhere.
Living with a commitment to excellence shall take you far.
Long life is in store for you.
Love is a warm fire to keep the soul warm.
Love is like sweet medicine, good to the last drop.
Love lights up the world.
Love truth, but pardon error.
Man is born to live and not prepared to live.
Man’s mind, once stretched by a new idea, never regains it’s original dimensions.
Many will travel to hear you speak.
Meditation with an old enemy is advised.
Miles are covered one step at a time.
Nature, time and patience are the three great physicians.
Never fear! The end of something marks the start of something new.
New ideas could be profitable.
New people will bring you new realizations, especially about big issues.
No one can walk backwards into the future.
Nothing is more difficult, and therefore more precious, than to be able to decide.
Now is a good time to buy stock.
Now is the time to go ahead and pursue that love interest!
Now is the time to try something new
Now is the time to try something new.
Observe all men, but most of all yourself.
One can never fill another’s shoes, rather he must outgrow the old shoes.
One of the first things you should look for in a problem is its positive side.
Others can help you now.
Pennies from heaven find their way to your doorstep this year!
People are attracted by your delicate features.
People find it difficult to resist your persuasive manner.
Perhaps you’ve been focusing too much on saving.
Physical activity will dramatically improve your outlook today.
Pick battles big enough to matter, small enough to win.
Place special emphasis on old friendship.
Please visit us at www.wontonfood.com
Po Says: Pandas like eating bamboo, but I prefer mine dipped in chocolate.
Practice makes perfect.
Protective measures will prevent costly disasters.
Put your mind into planning today. Look into the future.
Remember the birthday but never the age.
Remember to share good fortune as well as bad with your friends.
Rest has a peaceful effect on your physical and emotional health.
Resting well is as important as working hard.
Romance moves you in a new direction.
Savor your freedom – it is precious.
Say hello to others. You will have a happier day.
Sincerity is the finest point of communication.
Self-knowledge is a life long process.
Share your joys and sorrows with your family.
Sift through your past to get a better idea of the present.
Sloth makes all things difficult; industry all easy.
Small confidences mark the onset of a friendship.
Smile when you are ready.
Society prepares the crime; the criminal commits it.
Someone you care about seeks reconciliation.
Soon life will become more interesting.
Stand tall. Don’t look down upon yourself.
Staying close to home is going to be best for your morale today
Stop searching forever, happiness is just next to you.
Strong reasons make strong actions.
Stuff happens. It is your response that counts.
Success is a journey, not a destination.
Success is failure turned inside out.
Success is going from failure to failure without loss of enthusiasm.
Swimming is easy. Stay floating is hard.
Take care and sensitivity you show towards others will return to you.
Take the high road.
Technology is the art of arranging the world so we do not notice it.
The architect of your destiny is yourself.
The austerity you see around you covers the richness of life like a veil.
The best men are molded of their faults.
The best prediction of future is the past.
The change you started already have far-reaching effects. Be ready.
The first man gets the oyster, the second man gets the shell.
The greatest achievement in life is to stand up again after falling.
The harder you work, the luckier you get.
The night life is for you.
The one that recognizes the illusion does not act as if it is real.
The only people who never fail are those who never try.
The person who will not stand for something will fall for anything.
The philosophy of one century is the common sense of the next.
The saints are the sinners who keep on trying.
The secret to good friends is no secret to you.
The small courtesies sweeten life, the greater ennoble it.
The smart thing to do is to begin trusting your intuitions.
The strong person understands how to withstand substantial loss.
The sure way to predict the future is to invent it.
The truly generous share, even with the undeserving.
The value lies not within any particular thing, but in the desire placed on that thing.
The weather is wonderful.
There is a time for caution, but not for fear.
There is no mistake so great as that of being always right.
There is no wisdom greater than kindness.
There is not greater pleasure than seeing your lived ones prosper.
There’s no such thing as an ordinary cat.
Things don’t just happen; they happen just.
This week, you’ll receive the recognition you deserve.
Those who care will make the effort.
Time and patience are called for many surprises await you!.
Time is precious, but truth is more precious than time
To know oneself, one should assert oneself.
To the world you may be one person, but to one person you may be the world.
Today is the conserve yourself, as things just won’t budge.
Today, your mouth might be moving but no one is listening.
Tonight you will be blinded by passion.
Use your eloquence where it will do the most good.
We first make our habits, and then our habits make us.
Welcome change.
“Welcome” is a powerful word.
Well done is better than well said.
What’s hidden in an empty box?
What’s that in your eye? Oh…it’s a sparkle.
What’s yours in mine, and what’s mine is mine.
When more become too much. It’s same as being not enough.
When your heart is pure, your mind is clear.
Wish you happiness.
With age comes wisdom.
You (sic) adventure could lead to happiness.
You always bring others happiness.
You are a person of another time.
You are a talented storyteller.
You are admired by everyone for your talent and ability.
You are almost there.
You are busy, but you are happy.
You are generous to an extreme and always think of the other fellow.
You are going to have some new clothes.
You are in good hands this evening.
You are interested in higher education, whether material or spiritual.
You are modest and courteous.
You are never selfish with your advice or your help.
You are next in line for promotion in your firm.
You are offered the dream of a lifetime. Say yes!
You are open-minded and quick to make new friends.
You are solid and dependable.
You are soon going to change your present line of work.
You are talented in many ways.
You are the master of every situation.
You are very expressive and positive in words, act and feeling.
You are working hard.
You begin to appreciate how important it is to share your personal beliefs.
You can keep a secret.
You can see a lot just by looking.
You can’t steal second base and keep your foot on first.
You desire recognition and you will find it.
You have a deep appreciation of the arts and music.
You have a deep interest in all that is artistic.
You have a friendly heart and are well admired.
You have a shrewd knack for spotting insincerity.
You have a yearning for perfection.
You have an active mind and a keen imagination.
You have an ambitious nature and may make a name for yourself.
You have an unusual equipment for success, use it properly.
You have exceeded what was expected.
You have had a good start. Work harder!
You have the power to write your own fortune.
You have yearning for perfection.
You know where you are going and how to get there.
You look pretty.
You love challenge.
You love chinese food.
You make people realize that there exist other beauties in the world.
You never hesitate to tackle the most difficult problems.
You never know who you touch.
You only treasure what you lost.
You seek to shield those you love and like the role of provider.
You should be able to make money and hold on to it.
You should be able to undertake and complete anything.
You should pay for this check. Be generous.
You understand how to have fun with others and to enjoy your solitude.
You will always be surrounded by true friends.
You will always get what you want through your charm and personality.
You will always have good luck in your personal affairs.
You will be a great success both in the business world and society.
You will be blessed with longevity.
You will be pleasantly surprised tonight.
You will be sharing great news with all the people you love.
You will be successful in your work.
You will be traveling and coming into a fortune.
You will be unusually successful in business.
You will become a great philanthropist in your later years.
You will become more and more wealthy.
You will enjoy good health.
You will enjoy good health; you will be surrounded by luxury.
You will find great contentment in the daily, routine activities.
You will have a fine capacity for the enjoyment of life.
You will have gold pieces by the bushel.
You will inherit a large sum of money.
You will make change for the better.
You will soon be surrounded by good friends and laughter.
You will take a chance in something in near future.
You will travel far and wide, both pleasure and business.
You will travel far and wide,both pleasure and business.
Your abilities are unparalleled.
Your ability is appreciated.
Your ability to juggle many tasks will take you far.
Your biggest virtue is your modesty.
Your character can be described as natural and unrestrained.
Your difficulties will strengthen you.
Your dreams are never silly; depend on them to guide you.
Your dreams are worth your best efforts to achieve them.
Your efforts in July will lead to success.
Your energy returns and you get things done.
Your family is young, gifted and attractive.
Your first love has never forgotten you.
Your goal will be reached very soon.
Your happiness is before you, not behind you! Cherish it.
Your hard work will payoff today.
Your heart will always make itself known through your words.
Your home is the center of great love.
Your ideals are well within your reach.
Your infinite capacity for patience will be rewarded sooner or later.
Your leadership qualities will be tested and proven.
Your life will be happy and peaceful.
Your life will get more and more exciting.
Your love life will be happy and harmonious.
Your love of music will be an important part of your life.
Your loyalty is a virtue, but not when it’s wedded with blind stubbornness.
Your mentality is alert, practical, and analytical.
Your mind is creative, original and alert.
Your mind is your greatest asset.
Your moods signal a period of change.
Your quick wits will get you out of a tough situation.
Your reputation is your wealth.
Your success will astonish everyone.
Your talents will be recognized and suitably rewarded.
Your work interests can capture the highest status or prestige.
)FORTUNES";

// ---------- helpers ----------

String htmlEscape(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '&') o += "&amp;";
    else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;";
    else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

bool cleanText(const String& s) {  // no control characters (they would break the config file)
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (c < 0x20 || c == 0x7F) return false;
  }
  return true;
}

// Safe fortune file name: letters, digits, - and _ only, always ends in .txt
String cleanName(String n) {
  int cut = max(n.lastIndexOf('/'), n.lastIndexOf('\\'));
  if (cut >= 0) n = n.substring(cut + 1);
  int dot = n.lastIndexOf('.');
  if (dot > 0) n = n.substring(0, dot);
  String o;
  for (size_t i = 0; i < n.length() && o.length() < 24; i++) {
    char c = n[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_') o += c;
    else if (c == ' ') o += '_';
  }
  if (o.length() == 0) o = "fortunes";
  return o + ".txt";
}

// Turns text into plain ASCII as it is written to a file: curly quotes become
// straight ones, dashes become '-', other accents/emoji are dropped, CRs removed.
// Works byte by byte so it can run on uploads that arrive in chunks.
struct Folder {
  uint8_t pend[4];
  int have;
  int need;
};
Folder fo = {{0, 0, 0, 0}, 0, 0};

void foldReset() {
  fo.have = 0;
  fo.need = 0;
}

void foldByte(uint8_t b, File& out) {
  if (fo.need > 0) {  // in the middle of a multi-byte character
    if ((b & 0xC0) == 0x80) {
      fo.pend[fo.have++] = b;
      if (fo.have == fo.need) {
        if (fo.need == 3 && fo.pend[0] == 0xE2 && fo.pend[1] == 0x80) {
          switch (fo.pend[2]) {
            case 0x98: case 0x99: out.write((uint8_t)'\''); break;
            case 0x9C: case 0x9D: out.write((uint8_t)'"'); break;
            case 0x93: case 0x94: out.write((uint8_t)'-'); break;
            case 0xA6: out.print("..."); break;
            default: break;
          }
        } else if (fo.need == 2 && fo.pend[0] == 0xC2 && fo.pend[1] == 0xA0) {
          out.write((uint8_t)' ');  // non-breaking space
        }
        fo.have = 0;
        fo.need = 0;
      }
      return;
    }
    fo.have = 0;  // broken sequence: treat this byte as a fresh one
    fo.need = 0;
  }
  if (b < 0x80) {
    if (b == '\r') return;
    if (b == '\n') out.write((uint8_t)'\n');
    else if (b == '\t') out.write((uint8_t)' ');
    else if (b >= 0x20 && b != 0x7F) out.write(b);
    return;
  }
  int need = 0;
  if (b >= 0xC2 && b <= 0xDF) need = 2;
  else if (b >= 0xE0 && b <= 0xEF) need = 3;
  else if (b >= 0xF0 && b <= 0xF4) need = 4;
  if (need == 0) return;  // stray byte: drop
  fo.pend[0] = b;
  fo.have = 1;
  fo.need = need;
}

// Optional password for the web pages (user name "admin")
bool authed() {
  if (cfgAdmin.length() == 0) return true;
  if (server.authenticate("admin", cfgAdmin.c_str())) return true;
  server.requestAuthentication();
  return false;
}

// ---------- config (simple key=value lines, like the rickroll sketch) ----------

void saveConfig() {
  if (!fsOk) return;
  File f = LittleFS.open(CONFIG_PATH, "w");
  if (!f) {
    Serial.println("Could not write config");
    return;
  }
  f.print("ssid=");   f.print(cfgSsid);   f.print("\n");
  f.print("pass=");   f.print(cfgPass);   f.print("\n");
  f.print("active="); f.print(cfgActive); f.print("\n");
  f.print("admin=");  f.print(cfgAdmin);  f.print("\n");
  f.print("lucky=");  f.print(cfgLucky ? "1" : "0");  f.print("\n");
  f.print("title=");  f.print(cfgTitle ? "1" : "0");  f.print("\n");
  f.close();
}

void loadConfig() {
  if (!fsOk || !LittleFS.exists(CONFIG_PATH)) return;
  File f = LittleFS.open(CONFIG_PATH, "r");
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.endsWith("\r")) line.remove(line.length() - 1);
    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    String k = line.substring(0, eq);
    String v = line.substring(eq + 1);
    if (k == "ssid") cfgSsid = v;
    else if (k == "pass") cfgPass = v;
    else if (k == "active") cfgActive = v;
    else if (k == "admin") cfgAdmin = v;
    else if (k == "lucky") cfgLucky = (v == "1");
    else if (k == "title") cfgTitle = (v == "1");
  }
  f.close();
}

// ---------- fortunes ----------

std::vector<String> listFortuneFiles() {
  std::vector<String> out;
  File dir = LittleFS.open(FORTUNE_DIR);
  if (!dir || !dir.isDirectory()) return out;
  File f = dir.openNextFile();
  while (f) {
    if (!f.isDirectory()) {
      String n = f.name();
      if (n.endsWith(".txt")) out.push_back(n);
    }
    f = dir.openNextFile();
  }
  return out;
}

int countLines(const String& name) {
  File f = LittleFS.open(String(FORTUNE_DIR) + "/" + name, "r");
  if (!f) return 0;
  int n = 0;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    l.trim();
    if (l.length() && l[0] != '#') n++;
  }
  f.close();
  return n;
}

String nthLine(const String& name, int idx) {
  File f = LittleFS.open(String(FORTUNE_DIR) + "/" + name, "r");
  if (!f) return "";
  int n = 0;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    l.trim();
    if (!l.length() || l[0] == '#') continue;
    if (n == idx) {
      f.close();
      return l;
    }
    n++;
  }
  f.close();
  return "";
}

// Reads the "#" lines at the top of a fortune file:
//   # Dad Jokes      -> the banner text (first # line that is not a setting)
//   # lucky: off     -> never add lucky numbers for this file (on = always)
void readHeader(const String& name, String& heading, int& lucky) {
  heading = "";
  lucky = -1;
  File f = LittleFS.open(String(FORTUNE_DIR) + "/" + name, "r");
  if (!f) return;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    l.trim();
    if (l.length() == 0) continue;   // blank lines in the header are fine
    if (l[0] != '#') break;          // the header ends at the first real fortune
    String body = l.substring(1);
    body.trim();
    String lower = body;
    lower.toLowerCase();
    if (lower.startsWith("lucky:")) {
      String v = lower.substring(6);
      v.trim();
      if (v == "off" || v == "no" || v == "0" || v == "false") lucky = 0;
      else if (v == "on" || v == "yes" || v == "1" || v == "true") lucky = 1;
    } else if (heading.length() == 0 && body.length() > 0) {
      while (body.endsWith(".")) body.remove(body.length() - 1);  // a trailing full stop is dropped
      body.trim();
      if (body.length() > 32) body = body.substring(0, 32);
      heading = body;
    }
  }
  f.close();
}

void ensureFortunes() {
  if (!LittleFS.exists(FORTUNE_DIR)) LittleFS.mkdir(FORTUNE_DIR);
  if (listFortuneFiles().empty()) {
    File f = LittleFS.open(String(FORTUNE_DIR) + "/general.txt", "w");
    if (f) {
      foldReset();  // curly quotes, dashes etc. become plain ASCII, like uploads
      for (size_t i = 0; DEFAULT_FORTUNES[i]; i++) foldByte((uint8_t)DEFAULT_FORTUNES[i], f);
      f.close();
      Serial.println("Wrote starter fortunes to /fortunes/general.txt");
    }
  }
}

String pickFortune() {
  pickedHeading = "";
  pickedLucky = -1;
  std::vector<String> files = listFortuneFiles();
  if (cfgActive != "*") {  // one chosen file (falls back to all if it has gone missing)
    for (size_t i = 0; i < files.size(); i++) {
      if (files[i] == cfgActive) {
        files.clear();
        files.push_back(cfgActive);
        break;
      }
    }
  }
  std::vector<int> counts;
  int total = 0;
  for (size_t i = 0; i < files.size(); i++) {
    int c = countLines(files[i]);
    counts.push_back(c);
    total += c;
  }
  if (total == 0) return "";
  int target = esp_random() % total;
  for (size_t i = 0; i < files.size(); i++) {
    if (target < counts[i]) {
      readHeader(files[i], pickedHeading, pickedLucky);
      return nthLine(files[i], target);
    }
    target -= counts[i];
  }
  return "";
}

// ---------- web pages ----------

String pageHead() {
  return String(R"HTML(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="color-scheme" content="light">
<title>Fortune Printer</title>
<style>
:root{--bg:#fde9b4;--card:#fff6dc;--line:#e8cc8a;--ink:#222;--amber:#f5b21b;--pill:#fbdc88;--navy:#2b3a8a;--muted:#6b5a2e}
body{font-family:system-ui,sans-serif;background:var(--bg);color:var(--ink);max-width:560px;margin:0 auto;padding:16px 12px 40px}
.slip{background:
 linear-gradient(var(--navy),var(--navy)) left 8px top 8px/30px 7px no-repeat,
 linear-gradient(var(--navy),var(--navy)) right 8px top 8px/30px 7px no-repeat,
 linear-gradient(var(--navy),var(--navy)) left 8px bottom 8px/30px 7px no-repeat,
 linear-gradient(var(--navy),var(--navy)) right 8px bottom 8px/30px 7px no-repeat,#fffdf6;
 color:var(--navy);font-family:Georgia,'Times New Roman',serif;text-align:center;padding:26px 44px;margin:8px 0 16px;box-shadow:0 1px 4px rgba(0,0,0,.25)}
.slip h1{margin:0;font-size:1.6em;font-weight:normal}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin:12px 0}
.card h3{margin:0 0 8px}
input,button,select{font-size:1em;padding:9px;margin:4px 0;width:100%;box-sizing:border-box;border-radius:8px}
input,select,textarea{background:#fff;color:var(--ink);border:1px solid var(--line)}
button{background:var(--amber);color:#2a2112;border:0;font-weight:600;cursor:pointer}
button.del{background:#f3c1b6;color:#5a1a10}
a{color:var(--navy)}
small{color:var(--muted)}
.msg{background:var(--pill);color:#3b2e0a;padding:10px;border-radius:8px;font-weight:600}
</style></head><body><div class="slip"><h1>Fortune Printer</h1></div>)HTML");
}

void handleRoot() {
  if (!authed()) return;
  String s = pageHead();
  int m = server.arg("m").toInt();
  const char* note = nullptr;
  if (m == 1) note = "Printing a fortune...";
  else if (m == 2) note = "Saved.";
  else if (m == 3) note = "File deleted.";
  else if (m == 4) note = "Print source updated.";
  else if (m == 5) note = "Admin password updated.";
  else if (m == 6) note = "No file was chosen.";
  else if (m == 7) note = "Fortune slip options saved.";
  if (note) s += "<div class=\"msg\">" + String(note) + "</div>";

  s += "<div class=\"card\"><h3>Print</h3><form method=\"POST\" action=\"/print\">"
       "<button>Print a fortune now</button></form></div>";

  // Fortune files
  s += "<div class=\"card\"><h3>Fortunes</h3>";
  std::vector<String> files = listFortuneFiles();
  if (files.empty()) s += "<p>No fortune files yet.</p>";
  for (size_t i = 0; i < files.size(); i++) {
    String n = htmlEscape(files[i]);
    s += "<p><b>" + n + "</b> <small>(" + String(countLines(files[i])) + " fortunes)</small><br>"
         "<a href=\"/edit?f=" + n + "\">Edit</a> &middot; <a href=\"/download?f=" + n + "\">Download</a> &middot; "
         "<form method=\"POST\" action=\"/delete\" style=\"display:inline\" "
         "onsubmit=\"return confirm('Delete " + n + "?')\">"
         "<input type=\"hidden\" name=\"f\" value=\"" + n + "\">"
         "<button class=\"del\" style=\"width:auto;padding:2px 10px\">Delete</button></form></p>";
  }
  if (!files.empty()) {
    s += "<form method=\"POST\" action=\"/setactive\"><label>Print from:</label>"
         "<select name=\"active\"><option value=\"*\"" + String(cfgActive == "*" ? " selected" : "") +
         ">All files mixed</option>";
    for (size_t i = 0; i < files.size(); i++) {
      String n = htmlEscape(files[i]);
      s += "<option value=\"" + n + "\"" + String(cfgActive == files[i] ? " selected" : "") + ">" + n + "</option>";
    }
    s += "</select><button>Use this</button></form>";
  }
  s += "<form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">"
       "<input type=\"file\" name=\"f\" accept=\".txt,text/plain\">"
       "<button>Upload a .txt file</button></form>"
       "<p><a href=\"/edit\">Write a new file</a></p>";
  s += "<small>Storage: " + String(LittleFS.usedBytes() / 1024) + " KB used of " +
       String(LittleFS.totalBytes() / 1024) + " KB</small></div>";

  // Fortune slip options
  s += "<div class=\"card\"><h3>Fortune slip</h3><form method=\"POST\" action=\"/setlucky\">"
       "<label>Print the FORTUNE banner?</label>"
       "<select name=\"title\"><option value=\"1\"" + String(cfgTitle ? " selected" : "") + ">Yes</option>"
       "<option value=\"0\"" + String(cfgTitle ? "" : " selected") + ">No</option></select>"
       "<label>Add lucky numbers to the fortune?</label>"
       "<select name=\"lucky\"><option value=\"1\"" + String(cfgLucky ? " selected" : "") + ">Yes</option>"
       "<option value=\"0\"" + String(cfgLucky ? "" : " selected") + ">No</option></select>"
       "<button>Save</button></form></div>";

  // Network
  s += "<div class=\"card\"><h3>WiFi</h3>";
  if (WiFi.status() == WL_CONNECTED) {
    s += "<p>Home network: <b>" + htmlEscape(WiFi.SSID()) + "</b><br>Address: " +
         WiFi.localIP().toString() + " or http://" + HOST_NAME + ".local</p>";
  } else if (cfgSsid.length()) {
    s += "<p>Not connected to <b>" + htmlEscape(cfgSsid) + "</b> (check the name and password).</p>";
  } else {
    s += "<p>No home network saved yet.</p>";
  }
  s += "<p><small>Its own WiFi: " + String(AP_SSID) + " at http://192.168.4.1</small></p>";
  s += "<form method=\"POST\" action=\"/setwifi\">"
       "<input name=\"ssid\" list=\"nets\" placeholder=\"Network name\" required>"
       "<datalist id=\"nets\">" + scannedOptions + "</datalist>"
       "<input name=\"pass\" type=\"password\" placeholder=\"Password\">"
       "<button>Save and reconnect</button></form>";
  if (cfgSsid.length()) {
    s += "<form method=\"POST\" action=\"/clearwifi\"><button>Forget saved network</button></form>";
  }
  s += "</div>";

  // Admin password
  s += "<div class=\"card\"><h3>Admin password</h3>";
  s += cfgAdmin.length() ? "<p>A password is set (user name: admin).</p>"
                         : "<p>No password set: anyone on the WiFi can change things.</p>";
  s += "<form method=\"POST\" action=\"/setadmin\">"
       "<input name=\"pass\" type=\"password\" placeholder=\"New password (blank removes it)\">"
       "<button>Save password</button></form>"
       "<small>Forgotten? Hold the button for 8 seconds to clear it (this also forgets the WiFi).</small></div>";
  s += "</body></html>";
  server.send(200, "text/html", s);
}

// Shown while the device restarts; waits for it to come back, then returns to "/"
String restartPage(const String& text) {
  return pageHead() + "<div class=\"card\"><p>" + text + "</p><p><small>This page returns "
         "automatically. If it does not, open http://" + HOST_NAME + ".local or "
         "http://192.168.4.1</small></p></div>"
         "<script>setTimeout(function w(){fetch('/',{cache:'no-store'}).then(function(){location='/'})"
         ".catch(function(){setTimeout(w,1500)})},3000)</script></body></html>";
}

void handleSetWifi() {
  if (!authed()) return;
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.length() < 1 || ssid.length() > 32 || pass.length() > 63 || !cleanText(ssid) || !cleanText(pass)) {
    server.send(400, "text/plain", "Bad network name or password");
    return;
  }
  cfgSsid = ssid;
  cfgPass = pass;
  saveConfig();
  server.send(200, "text/html", restartPage("Saved. Restarting to join the network..."));
  pendingRestart = true;
  restartAt = millis() + 800;
}

void handleClearWifi() {
  if (!authed()) return;
  cfgSsid = "";
  cfgPass = "";
  saveConfig();
  server.send(200, "text/html", restartPage("Network forgotten. Restarting..."));
  pendingRestart = true;
  restartAt = millis() + 800;
}

void handlePrint() {
  if (!authed()) return;
  printRequested = true;  // printing happens in loop() so this page answers straight away
  server.sendHeader("Location", "/?m=1");
  server.send(302, "text/plain", "");
}

// ---------- fortune file pages ----------

void redirectTo(const String& where) {
  server.sendHeader("Location", where);
  server.send(302, "text/plain", "");
}

String safePath(const String& name) {
  return String(FORTUNE_DIR) + "/" + name;
}

void handleEdit() {
  if (!authed()) return;
  String name = server.arg("f");
  bool isNew = (name.length() == 0);
  size_t fileSize = 0;
  if (!isNew) {
    name = cleanName(name);
    File f = LittleFS.open(safePath(name), "r");
    if (!f) {
      redirectTo("/");
      return;
    }
    fileSize = f.size();
    f.close();
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent(pageHead());

  if (fileSize > EDIT_MAX_BYTES) {
    server.sendContent("<div class=\"card\"><p>" + htmlEscape(name) + " is too large for the editor here. "
                       "Download it, edit it on your computer, then upload it again.</p>"
                       "<p><a href=\"/\">Back</a></p></div></body></html>");
    server.sendContent("");
    return;
  }

  String title = isNew ? String("New fortune file") : String("Edit ") + htmlEscape(name);
  String h = "<div class=\"card\"><h3>" + title + "</h3><form method=\"POST\" action=\"/save\">";
  if (isNew) h += "<input name=\"name\" placeholder=\"File name, e.g. spooky\" required>";
  else h += "<input type=\"hidden\" name=\"name\" value=\"" + htmlEscape(name) + "\">";
  h += "<textarea name=\"text\" rows=\"18\" style=\"width:100%;box-sizing:border-box;font-size:1em\">";
  server.sendContent(h);

  if (!isNew) {
    File f = LittleFS.open(safePath(name), "r");
    String buf;
    while (f && f.available()) {
      buf += htmlEscape(f.readStringUntil('\n')) + "\n";
      if (buf.length() > 1000) {
        server.sendContent(buf);
        buf = "";
      }
    }
    if (f) f.close();
    if (buf.length()) server.sendContent(buf);
  }

  server.sendContent("</textarea><small>One fortune per line. Type \\n inside a line for a line break (\\n\\n for a gap). "
                     "Start the file with # Heading to change the banner, and # lucky: off to skip lucky numbers. "
                     "Curly quotes are converted and accents are dropped, because the printer font is plain ASCII.</small>"
                     "<button>Save</button></form><p><a href=\"/\">Cancel</a></p></div></body></html>");
  server.sendContent("");
}

void handleSave() {
  if (!authed()) return;
  String name = cleanName(server.arg("name"));
  String text = server.arg("text");
  if (text.length() > EDIT_MAX_BYTES) {
    server.send(413, "text/plain", "Too big for the editor. Upload the file instead.");
    return;
  }
  File f = LittleFS.open(safePath(name), "w");
  if (!f) {
    server.send(500, "text/plain", "Could not write the file");
    return;
  }
  foldReset();
  for (size_t i = 0; i < text.length(); i++) foldByte((uint8_t)text[i], f);
  f.close();
  redirectTo("/?m=2");
}

void handleDelete() {
  if (!authed()) return;
  String name = cleanName(server.arg("f"));
  LittleFS.remove(safePath(name));
  if (cfgActive == name) {
    cfgActive = "*";
    saveConfig();
  }
  redirectTo("/?m=3");
}

void handleSetActive() {
  if (!authed()) return;
  String a = server.arg("active");
  if (a != "*") {
    a = cleanName(a);
    if (!LittleFS.exists(safePath(a))) a = "*";
  }
  cfgActive = a;
  saveConfig();
  redirectTo("/?m=4");
}

void handleSetLucky() {
  if (!authed()) return;
  cfgLucky = (server.arg("lucky") == "1");
  if (server.hasArg("title")) cfgTitle = (server.arg("title") == "1");
  saveConfig();
  redirectTo("/?m=7");
}

void handleSetAdmin() {
  if (!authed()) return;
  String p = server.arg("pass");
  if (p.length() > 32 || !cleanText(p)) {
    server.send(400, "text/plain", "Password too long or has odd characters");
    return;
  }
  cfgAdmin = p;  // blank removes the password
  saveConfig();
  redirectTo("/?m=5");
}

void handleDownload() {
  if (!authed()) return;
  String name = cleanName(server.arg("f"));
  File f = LittleFS.open(safePath(name), "r");
  if (!f) {
    redirectTo("/");
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
  server.streamFile(f, "text/plain");
  f.close();
}

// Upload: runs once per chunk while the file arrives, cleaning it on the way in
void handleUploadData() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    uploadSeen = true;
    uploadFailed = false;
    uploadBytes = 0;
    foldReset();
    if (uploadFile) uploadFile.close();
    if (cfgAdmin.length() && !server.authenticate("admin", cfgAdmin.c_str())) {
      uploadFailed = true;  // not logged in: write nothing
      return;
    }
    uploadName = cleanName(up.filename);
    uploadFree = LittleFS.totalBytes() - LittleFS.usedBytes();
    uploadFile = LittleFS.open(safePath(uploadName), "w");
    if (!uploadFile) uploadFailed = true;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (uploadFailed || !uploadFile) return;
    uploadBytes += up.currentSize;
    if (uploadBytes > MAX_FILE_BYTES || uploadBytes + 4096 > uploadFree) {  // too big or no room
      uploadFailed = true;
      uploadFile.close();
      LittleFS.remove(safePath(uploadName));
      return;
    }
    for (size_t i = 0; i < up.currentSize; i++) foldByte(up.buf[i], uploadFile);
  } else if (up.status == UPLOAD_FILE_END) {
    if (uploadFile) uploadFile.close();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (uploadFile) uploadFile.close();
    LittleFS.remove(safePath(uploadName));
    uploadFailed = true;
  }
}

void handleUploadDone() {
  if (!authed()) return;
  if (!uploadSeen) {  // the form was sent with no file chosen
    redirectTo("/?m=6");
    return;
  }
  uploadSeen = false;
  if (uploadFailed) {
    server.send(400, "text/plain", "Upload failed: file too big (100 KB max), not enough space, or not logged in.");
    return;
  }
  redirectTo("/?m=2");
}

// ---------- WiFi ----------

void setupWiFi() {
  WiFi.persistent(false);
  WiFi.setHostname(HOST_NAME);

  // Scan before the access point starts, so the setup page can offer a list
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  scannedOptions = "";
  for (int i = 0; i < n && i < 20; i++) {
    scannedOptions += "<option value=\"" + htmlEscape(WiFi.SSID(i)) + "\">";
  }
  WiFi.scanDelete();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  Serial.print("Own WiFi '");
  Serial.print(AP_SSID);
  Serial.print("' at ");
  Serial.println(WiFi.softAPIP());

  if (cfgSsid.length()) {
    Serial.printf("Joining '%s'", cfgSsid.c_str());
    WiFi.begin(cfgSsid.c_str(), cfgPass.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Connected, IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Could not join (will keep retrying; own WiFi still works)");
    }
  } else {
    Serial.println("No home network saved. Join 'FortunePrinter' and open http://192.168.4.1");
  }
}

// ---------- setup / loop ----------

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  fsOk = LittleFS.begin(true);  // true = format on first use
  Serial.println(fsOk ? "LittleFS mounted" : "LittleFS mount FAILED");
  if (fsOk) {
    loadConfig();
    ensureFortunes();
  }

  setupWiFi();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/setwifi", HTTP_POST, handleSetWifi);
  server.on("/clearwifi", HTTP_POST, handleClearWifi);
  server.on("/print", HTTP_POST, handlePrint);
  server.on("/edit", HTTP_GET, handleEdit);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/setactive", HTTP_POST, handleSetActive);
  server.on("/setadmin", HTTP_POST, handleSetAdmin);
  server.on("/setlucky", HTTP_POST, handleSetLucky);
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/upload", HTTP_POST, handleUploadDone, handleUploadData);
  const char* wantedHeaders[] = {"Authorization"};
  server.collectHeaders(wantedHeaders, 1);
  server.onNotFound([]() { server.sendHeader("Location", "/"); server.send(302, "text/plain", ""); });
  server.begin();
  Serial.println("Ready. Tap the button for a fortune.");
}

void loop() {
  server.handleClient();

  // Keep the fortune.local name alive as the home network comes and goes
  static bool mdnsStarted = false;
  static bool wasUp = false;
  bool up = (WiFi.status() == WL_CONNECTED);
  if (!mdnsStarted || up != wasUp) {
    wasUp = up;
    if (mdnsStarted) MDNS.end();
    if (MDNS.begin(HOST_NAME)) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      Serial.println("Name: http://fortune.local");
    }
  }

  // Button: tap (under 2 s) = print, hold 8 s = forget saved WiFi
  static bool down = false;
  static bool longDone = false;
  static unsigned long downSince = 0;
  static unsigned long lastEdge = 0;
  bool nowDown = (digitalRead(BUTTON_PIN) == LOW);
  if (nowDown != down && millis() - lastEdge > 30) {
    lastEdge = millis();
    down = nowDown;
    if (down) {
      downSince = millis();
      longDone = false;
    } else if (!longDone && millis() - downSince < 2000) {
      printRequested = true;
    }
  }
  if (down && !longDone && millis() - downSince > 8000) {
    longDone = true;
    cfgSsid = "";
    cfgPass = "";
    cfgAdmin = "";
    saveConfig();
    Serial.println("Saved WiFi and admin password forgotten, restarting");
    pendingRestart = true;
    restartAt = millis() + 500;
  }

  if (printRequested) {
    printRequested = false;
    String f = pickFortune();
    if (f.length()) {
      Serial.println(f);
      g_luckyNumbers = (pickedLucky < 0) ? cfgLucky : (pickedLucky == 1);
      g_showTitle = cfgTitle;
      g_heading = pickedHeading.length() ? pickedHeading : String("FORTUNE");
      printFortune(f);
    } else {
      Serial.println("No fortunes found in /fortunes");
    }
  }

  if (pendingRestart && millis() > restartAt) ESP.restart();
  delay(2);
}
