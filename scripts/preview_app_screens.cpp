#include <U8g2_for_Adafruit_GFX.h>
#include <string>
Adafruit_GFX g;U8G2_FOR_ADAFRUIT_GFX f;std::string out;
void t(int x,int y,const char*s,bool bold=false,bool inverse=false){f.setFont(bold?u8g2_font_helvB10_te:u8g2_font_helvR10_te);f.setForegroundColor(inverse?0:1);f.setBackgroundColor(inverse?1:0);f.setCursor(x,y);f.print(s);}
void h(int x,int y,const char*s){f.setFont(u8g2_font_helvB12_te);f.setForegroundColor(1);f.setBackgroundColor(0);f.setCursor(x,y);f.print(s);}
void big(int x,int y,const char*s){f.setFont(u8g2_font_helvB18_te);f.setForegroundColor(1);f.setBackgroundColor(0);f.setCursor(x,y);f.print(s);}
void pill(int x,int y,int w,const char*s){g.fillRoundRect(x,y,w,26,13,1);t(x+12,y+19,s,true,true);}
void start(const char*label,const char*right=""){memset(g.pixels,0,sizeof(g.pixels));h(7,21,label);f.setFont(u8g2_font_helvR10_te);int w=f.getUTF8Width(right);t(243-w,20,right);}
void save(const char*name){g.save((out+"/"+name+".pbm").c_str());}
void rows(const char*a,const char*b,const char*c){pill(5,33,240,a);t(17,82,b);t(17,110,c);}
int main(int argc,char**argv){if(argc!=2)return 1;out=argv[1];f.begin(g);f.setFontMode(1);
start("Bibliothek","99%");big(8,52,"Der kleine");big(8,78,"Prinz");pill(5,92,159,"Lesen · 42%");t(182,111,"2 / 8");save("library");
start("Bücher","2 / 8");rows("Der kleine Prinz","Die Verwandlung","Romane");save("books");
start("Romane","1 / 3");rows("Der Steppenwolf","Siddhartha","Zur Bibliothek");save("folder");
start("Mehr","1 / 3");rows("Lesezeichen","Listen","Apps");save("more");
start("Mehr","2 / 3");rows("Statistik","Einstellungen","Bücher übertragen");save("more2");
start("Mehr","3 / 3");rows("Gerät & Version","Firmware-Update","Zur Bibliothek");save("more3");
memset(g.pixels,0,sizeof(g.pixels));t(6,15,"Heute nehme ich mir Zeit.");t(6,34,"Die Werkstatt ist still, und");t(6,53,"auf dem Tisch liegt das Buch.");t(6,72,"Ich schlage eine Seite auf");t(6,91,"und lese einfach weiter.");t(6,118,"Seite 84");t(208,118,"42%");save("reader");
start("Dein Buch","42%");h(8,48,"Der kleine Prinz");t(8,69,"Seite 84");pill(5,88,154,"Weiterlesen");t(178,108,"Menü");save("reading");
start("Lesemenü","1 / 2");rows("Lesezeichen setzen","Schrift & Abstände","Statusleiste");save("readmenu");
start("Lesemenü","2 / 2");rows("Buch schließen","Gerät sperren","Zurück zum Lesen");save("readmenu2");
start("Lesezeichen","1 / 3");rows("Der kleine Prinz","Die Verwandlung","Zur Bibliothek");save("bookmarkbooks");
start("Der kleine Prinz","1 / 2");pill(5,33,240,"Seite 84");t(17,81,"Seite 126");t(17,110,"Zurück");save("bookmarks");
start("Seite 84","Lesezeichen");t(8,48,"Ein guter Gedanke braucht");t(8,68,"manchmal nur einen ruhigen");t(8,88,"Moment und eine freie Seite.");save("preview");
start("Listen","1 / 3");rows("Werkstatt","Einkaufen","Projekte");save("lists");
start("Werkstatt","1 / 3");rows("Schleifpapier holen","Holz zuschneiden","Oberfläche ölen");save("list");
start("Apps","1 / 3");rows("Notizen","Rechner","Zur Bibliothek");save("apps");
start("Statistik","Heute");big(8,59,"24 Seiten");t(8,83,"7 Tage in Folge gelesen");t(8,111,"Gesamt: 1.248 Seiten");save("stats");
start("Einstellungen","1 / 2");rows("Lesen","Tasten","Ruhe & Sperre");save("settings");
start("Einstellungen","2 / 2");rows("Statusleiste","WLAN & Übertragen","Zurück");save("settings2");
start("Statusleiste");rows("Vollständig","Nur Fortschritt","Ausblenden");save("statusbar");
start("Schrift & Abstände");pill(5,33,240,"Helvetica · 10");t(17,82,"Zeilenabstand     0");t(17,110,"Bionic Reading    Aus");save("font");
start("Tasten");pill(5,33,240,"Reaktion · Schnell");t(17,82,"Einfach: Weiter");t(17,110,"Doppelt: Zurück");save("buttons");
start("Ruhe & Sperre");pill(5,33,240,"Ruhemodus · 5 Min.");t(17,82,"Automatisch sperren");t(17,110,"Ruhebild anzeigen");save("sleepconfig");
start("Übertragen","WLAN");h(8,49,"Mit Pala verbinden");t(8,70,"Netz: Pala-One");pill(5,88,240,"192.168.4.1");save("wifi");
start("Verbunden","WLAN");h(8,49,"Bücher hinzufügen");t(8,71,"Öffne im Browser:");pill(5,88,240,"192.168.1.42");save("upload");
start("Gerät");h(8,51,"Pala One");t(8,77,"Heltec V1.2");t(8,105,"Firmware · Lokal");save("device");
start("Firmware-Update");h(8,52,"Lokal aktualisieren");t(8,76,"Datei im Browser wählen");pill(5,89,240,"Update öffnen");save("update");
start("Update läuft");big(8,60,"68%");g.fillRoundRect(8,82,234,7,3,1);g.fillRoundRect(168,82,74,7,3,0);t(8,116,"Pala eingeschaltet lassen");save("updating");
start("Pala One");big(8,62,"Gesperrt");t(8,91,"Taste halten zum Entsperren");save("lock");
start("Pala One");big(8,62,"Bis gleich.");t(8,91,"Drücken zum Weiterlesen");save("sleep");
start("Deine Bibliothek");h(8,53,"Platz für Geschichten.");t(8,76,"Füge dein erstes Buch hinzu.");pill(5,89,240,"Bücher übertragen");save("empty");
start("Speicher");h(8,51,"Nicht verfügbar");t(8,74,"Deine Daten bleiben erhalten.");pill(5,89,240,"Neu starten");save("error");
}
