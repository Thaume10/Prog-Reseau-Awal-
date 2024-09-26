This project is a multiplayer version of the traditional Awalé game, developed in C. The game allows players to challenge each other, play in parallel, stream matches to spectators, and interact via an integrated chat system. The application uses networking to support online play between multiple users, and players can manage their game sessions, view match history, and more.

# Features
  User Login: Players can connect using a custom pseudonym.
  Challenge System: Users can challenge other online players, accept or decline challenges, and view the list of available players.
  Awalé Gameplay: Players can play Awalé by choosing the game direction. Invalid moves, such as selecting empty pits, are managed.
  Simultaneous Games: Users can play multiple games at the same time.
  Game Streaming & Chat: Spectators can watch live games and chat during the match. There is also a global chat system for all users.
  Session Management: Players can disconnect at any time. Server data is saved in JSON format when the server shuts down.
  
# Recent Modifications
  Players can disconnect when desired.
  Server data is stored in a JSON file upon shutdown.
# Known Issues
  The server does not yet load from the JSON file on startup.
  Handling of exceptional disconnections is incomplete due to the complexity of debugging threads.
# How to Run
  Navigate to the project directories and run make in both the server and client folders.
  To start the server, run:    ./server2
  To start a client, run:    ./client2 127.0.0.1 "your_pseudo"
# Technical Solutions
  Threading and Mutexes: We used threads to manage multiple simultaneous connections and games. While threading added complexity, it allowed us to manage concurrent sessions and player actions.
  Game Data Structure: Each game session consists of a board, turn direction, score, and the current player. Challenges, clients, and chat systems are managed using shared lists.
