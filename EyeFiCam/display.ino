void displayTimeAndDate(){
  DateTime now = clock.now();
  if(now.hour() >= 12) {
    hour = now.hour() - 12;
    sprintf(amOrPm,"%s","PM");   
  } else {
    hour = now.hour();
    sprintf(amOrPm,"%s","AM");   
  }
  sprintf(buffer1,"%02d:%02d:%02d %s",hour,now.minute(), now.second(),amOrPm );   
  sprintf(buffer2,"%02d/%02d/%4d",now.month(), now.day(), now.year());   
  if(second != now.second()) {
    display(buffer1, buffer2);
    second = now.second();
  }
}


void display(String line1, String line2) {
  lcd.setCursor(0,0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}






