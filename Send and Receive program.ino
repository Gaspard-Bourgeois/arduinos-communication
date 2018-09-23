//Définition des pins
const char digitalOutput[1]={3}; //pin d'émission pour la carte émetrice
const char digitalInput[2]={4,5}; //pins de réception pour la carte réceptrice

//Mesure du temps
unsigned long t0=millis();  //Variable permettant de stocker l'instant de la dernière lecture
unsigned long t;  //Variable stockant le temps à chaque instant

//En-tête du message
const unsigned char identifying=207;  //identifiant propre à la communication
const unsigned char key=15; //clé permettant de certifier l'authenticité de l'émetteur
bool RXHLast[16]; //On stocke les 16 dernières mesures lues afin de vérifier la correspondance entre les valeurs lues et le header attendu

//Corps du message
unsigned char checksum; //somme décimale du titre et du message à envoyer
char checksum_5bits;  //valeur décimale du checksum modulo 32 permettant son envoi sur 5 bits
unsigned char RXBody[3];  //valeurs décimales respectivement du titre, du message et du checksum recu

//Ensemble des couples de titre et message à envoyer (titre<16;message<127)
const int request[10][2]={{1,1},{1,2},{1,3},{1,4},{1,5},{1,6},{1,7},{1,8},{1,9},{1,10}};

//Paramètres d'envois
const char bitTime= 10; //durée d'émission de chaque bits en ms
const char requestBreak=2;  //nombre de bits d'attente à la fin de l'émission de chaque requête permettant du temps de calcul au programme récepteur
const char requestRedundancy=2; //nombre d'envois successifs de la requête
const char headerRedundancy=1; //nombre d'envois successifs de l'en-tête
const char bodyRedundancy=1;  //nombre d'envois successifs du corps de message



void setup()
{
  for(int i=0;i<int(sizeof(digitalOutput));i++)
  {
    pinMode(digitalOutput[i], OUTPUT);  //définition des pins de sorties
  }
  for(int i=0;i<int(sizeof(digitalInput));i++)
  {
    pinMode(digitalInput[i], INPUT);  //définition des pins d'entrées digitales
  }
    Serial.begin(115200);
  
  //Vérification de la conformité de longueur des titres et des messages à transmettre
  if(check_size(request))
  {
    Serial.print("Les données à envoyer ne respectent pas les dimensions qui leur sont alouées");
  }

  //Affichage des paramètres choisis dans le moniteur serie
  Serial.print(bitTime*1);
  Serial.print("-");
  Serial.print(requestBreak*1);
  Serial.print("-");
  Serial.print(requestRedundancy*1);
  Serial.print("-");
  Serial.println(bodyRedundancy*1);

  //Affichage du débit
  Serial.print("Temps d'emission d'une requête: ");
  Serial.println(bitTime*requestRedundancy*(16*(1+bodyRedundancy)+requestBreak));
}



void loop()
{
/*1.RX():
 * Afin d'actionner la carte en mode réception
 * 
 * 2.TX():
 * Afin d'actionner la carte en mode émission
 */
TX();
}


//Gestion de la réception
void RX()
{/*Gestion de la reception et traitement des données recues*/
  bool flag=0;  //Drapeau commandant l'affichage ou non des informations recues
  int checksumValid=0; //Compteur du nombre de checksum recu cohérant avec le titre et le message
  unsigned char title=0;  //Valeur décimale coherente du titre recu
  unsigned char message=0;  //Valeur décimale cohérente du message recu
  
  for(int i=0;i<requestRedundancy;i++)
  {
    for(int j=0;j<headerRedundancy;j++)
    {
      RX_header();  //Réception de l'en-tête la requête 
    }
  
    for(int j=0;j<bodyRedundancy;j++)
    {   
      RX_body();  //Réception du corps de la requête
      
      //Vérification de la cohérence des résultats
      if(((RXBody[0]+RXBody[1])%32)==RXBody[2] & RXBody[0]!=0 & RXBody[1]!=0)
      {
        checksumValid+=1;
        //On commande l'affichage des résultats si tous les titres et messages cohérents sont identiques
        flag=!((checksumValid!=1)*!((checksumValid>=2)*(checksumValid<=bodyRedundancy*requestRedundancy)*(title==RXBody[0])*(message==RXBody[1])*(flag==1))); 
        title=RXBody[0]; //Le titre est mis à zéro si le drapeau est baissé
        message=RXBody[1];
      }
    }
  }
  //Affichage du résultat si l'affichage est demandé et le titre est non nul
  if(flag==1)
  {
    Serial.print(title);
    Serial.print("-");
    Serial.println(message);
  }
}


//Réception de l'en-tête de la requête
void RX_header()
{/*Analyse les valeurs recues et continu à la lecture d'un en-tête valide*/
  t0=millis();
  t=t0;
  unsigned int RXHCounter=0;//décompte du nombre de valeurs lues
  for(int i=0; i<16;i++)//initialisation des dernières valeurs lues
  {
    RXHLast[i]=0;
  }
  //Recherche jusqu'à trouver un signal header correct
  while(!(!RXHLast[0]*!RXHLast[1]*RXHLast[2]*RXHLast[3]*RXHLast[4]*RXHLast[5]*!RXHLast[6]*!RXHLast[7]*RXHLast[8]*RXHLast[9]*!RXHLast[10]*!RXHLast[11]*RXHLast[12]*RXHLast[13]*RXHLast[14]*RXHLast[15]))
  {
    t=millis();
    //Traitement des signaux cadencé
    if(t-t0>=bitTime)
    {
      t0=t;
      for(int i=15; i>0;i--)
      {
        RXHLast[i]=RXHLast[i-1]; //Décalage des derniers bits recus
      }
      RXHLast[0]=reader_dig();  //Lecture et enregistrement du signal recu
      RXHCounter+=1;  //Incrementation du nombre de bits recu
      
      //Resynchronisation de la tête de lecture en cas d'attente trop longue
      if(RXHCounter>(16*(headerRedundancy+bodyRedundancy+1)+2*requestBreak+1))//il doit d'abord lire 16 valeurs avant de pouvoir comparer avec le bon code puis il doit ignorer les bits de break deux fois
      {
        t0-=int(bitTime/3); //On diminue le dernier temps sauvegarder afin de maximiser les chances de synchronisation et ne pas perdre de données
        RXHCounter=0; //Remise du compteur de bits recu à zéro
        Serial.println("Decalage de la tete de lecture");
      }
    }
  }
}


//Réception du corps de la requête
void RX_body()
{/*Dechiffre les signaux recus et les associes à chaque données*/
  char j=0; //indice du type de donné recu (titre/message/checksum)
  char k=0; //poids du bit recu pour chaque type de message recu
  bool condition[2];  //conditions de changements de type de message
  
  //Remise à zéro des informations recues
  for(int l=0;l<3;l++)
  {
    RXBody[l]=0;
  }

  //Tant que le checksum(en 3e position, codé sur 5 bits) n'a pas fini d'être lu 
  while(k<5 || j<2)
  {
    t=millis();
    //Traitement des signaux cadencés
    if(t-t0>=bitTime)
    {
      t0=t;
      RXBody[j]+=bit(k)*reader_dig(); //incrémentation de la valeur décimale du poids du bit reçu au type de message convenu
      k+=1; //incrémentation du poid du bits
      
      //Si le titre a fini d'être lu on remet à zéro le poids du bit et on change de type de variable lue
      condition[0]=(k>=4)*(j==0);
      k*=!condition[0];
      j+=condition[0];
      
      //Si le message a fini d'être lu on remet à zéro le poids du bit et on change de type de variable lue
      condition[1]=(k>=7)*(j==1);
      k*=!condition[1];
      j+=condition[1];
    }
  }
}


//Lecture de l'entrée analogique
bool reader_dig()
{/*Lit la valeur digital sur la pin d'entrée*/
  return digitalRead(digitalInput[1]);
}


//Gestion de l'émission
void TX()
{/*Metode d'envoie cadencé, dépendant des paramètres d'envoies définis*/
  for(int i=0;i<int(sizeof(request)/4);i++) //on lit une fois chaque couple de variable à envoyer
  {
    for(int j=0; j<requestRedundancy; j++)
    {
      for(int l=0;l<headerRedundancy;l++)
      {
        TX_header();  //Envoi de l'en-tête
      }
      for(int l=0;l<bodyRedundancy;l++)
      {
        TX_body(request[i][0],request[i][1]); //Envoi du corps
      }
      //Ajout d'un délai avant l'émission de la prochaine en-tête
      delay(requestBreak*bitTime);
    }
  }
} 


//Emission de l'en-tête de la requête
void TX_header()
{/*Emission de l'identifiant et de la clé de communication*/
  convert_send(identifying,10);
  convert_send(key,6);
}


//Emission du corps de la requête
void TX_body(unsigned char title, unsigned char message)
{/*Emission des 3 types de données*/
  //création du checksum codé sur 5 bits
  checksum= title + message;
  checksum_5bits=checksum%32;
  
  convert_send(title,4); //conversion binaire et envoi du titre codé sur 4 bits
  convert_send(message,7); //conversion binaire et envoi du message codé sur 7 bits
  convert_send(checksum_5bits,5); //conversion binaire et envoi du checksum codé sur 5 bits
}
  
  
//Conversion et envoi des signaux
void convert_send(unsigned char content, char contentBinSize)
{/*Convertit en binaire et envoie le tout de facon cadencé*/
  for(int i=0;i<contentBinSize;i++)
  {
    send(content%2);
    content=content/2;
  }
}

//Envoie des signaux
void send(bool value)
{/*Envoie les bits de façon cadencé*/
  digitalWrite(digitalOutput[0],value);
  delay(bitTime);
}
  

bool check_size(const int array[][2])
{/*Verifie que les colonnes contiennent des nombres respectivement inférieurs à 16 et 128*/
  bool flag;
  for(int i=0;i<int(sizeof(array)/4);i++)
  {//rappel: flag est un boolean
    flag+=array[i][0]>=16;
    flag+=array[i][1]>=128;
  }
  return flag;
}
  
  
