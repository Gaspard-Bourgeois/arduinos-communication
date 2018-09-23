# arduinos-communication
Exchange of data between two Arduinos with redundancy and checksum verification

The code aimed to make two Arduinos exchange a number randomly generated.
The number is converted into a binary signal. A checksum is added at the end of the signal. You can parameter the length of the signal and the number of repetitions of the sequence.

Every communication is preceded by a "header" a sequence announcing the beginning of the communication. You can also parameter the number of headers.

The code was used in a personal project than you can see in my personal portfolio.
[More information about the Project](https://gaspard-bourgeois.github.io/conception-of-a-wireless-system-imitating-a-pacemaker/)
