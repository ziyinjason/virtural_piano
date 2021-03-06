/*
24-780 Eng Comp
Final Project: Mr Zero Virtual Piano
Component: User Input
Matthew Bartnof
2018 Nov 30

2018 11 30 General housekeeping
- Added more comments
- Commented out unnecessary lines of code, but did not delete
- Commented out unnecessary functions, but did not delete
*/

#include "IO.h"

void overallPiano::checkModeChanged(void) {
	// Check mouse options to change modes

	int leftButton, middleButton, rightButton, mouseX, mouseY;
	int mouseEvent;
	int cX, cY, radius;
	float distance;

	modeChanged = false;	// assume mode does not change

	mouseEvent = FsGetMouseEvent(leftButton, middleButton, rightButton, mouseX, mouseY);	// get user's mouse inputs
	if (mouseEvent == FSMOUSEEVENT_LBUTTONDOWN) {
		// Upon left click, check if its in a valid mode-changing region
		for (int checkingMode = 0; checkingMode < qtyModes; checkingMode++) {
			getModeButtonPosition(checkingMode, cX, cY, radius);
			distance = (float)((cX - mouseX)*(cX - mouseX)) + (float)((cY - mouseY)*(cY - mouseY));

			if (distance < radius*radius) {
				// if it's inside a valid region, change the mode
				setCurrentMode(checkingMode);
			}
		}
	}
}

void overallPiano::checkKeyStates(void) {
	/*
	Check keyboard

	Will temporarily record the old value for each key.
	If the value changes, it will record that in notesPlaying
	so audio can play the note
	*/

	bool keyWasDepressed[15];

	std::memcpy(keyWasDepressed, userKeyDepressed, 15);						// record old state

	for (int i = 0; i < 15; i++) {												// For each note
		userKeyDepressed[i] = FsGetKeyState(orderOfComputerKeys[i]);			//   Get its state: 1=pressed, 0=unpressed
		userKeyFirstPressed[i] = (!keyWasDepressed[i] && userKeyDepressed[i]);	//   If it was not pressed last iteration, but is now, set userKeyFirstPressed = TRUE
	}
}

void overallPiano::setCurrentMode(int newMode)
{
	// Set current mode to input parameter

	modeChanged = true;
	currentMode = newMode;
}

void overallPiano::resetSongNotes(void)
{
	// Fill song instruction array with FALSE (clear out entirely)

	std::fill_n(songNotes, 15, 0);
}


int overallPiano::checkValidNote(std::string noteToCheck)
{
	// Checks if a string passed in is a valid note for the piano.
	// If so, it will return the position on the piano (i.e., the index of the note)

	// Look for the noteToCheck within the full array of piano notes available (i.e. orderOfPianoKeys)
	std::vector<std::string>::iterator i = std::find(orderOfPianoKeys.begin(), orderOfPianoKeys.end(), noteToCheck);
	if (i != orderOfPianoKeys.end()) {					// if noteToCheck exists within the array
		return distance(orderOfPianoKeys.begin(), i);	//		return its index
	}
	else {												// Otherwise
		return -1;										//		return -1
	}
}

int overallPiano::getCurrentMode(void)
{
	return currentMode;
}

bool overallPiano::didModeChange(void)
{
	return modeChanged;
}

bool overallPiano::didUserESC(void)
{
	return userESC;
}

int overallPiano::howManyModes(void)
{
	return qtyModes;
}

bool overallPiano::didUserMakeInput(void)
{
	// Check to see if user made a significant input (changed keyboard input, select new mode, presses ESC)

	// To check if keyboard input changed, compare this iterations userKeyFirstPressed to the last iteration
	// Therefore, need to temporarily store last iterations userKeyFirstPressed
	bool oldFirstPressed[15];
	std::memcpy(oldFirstPressed, userKeyFirstPressed, 15);

	// Check new user inputs
	getUserInput();

	// If the new key state is different from last time, return true
	for (int i = 0; i < 15; i++) {
		if (userKeyFirstPressed[i]) {
			return true;
		}
	}


	// keyboard state is the same so now just check mouse/ESC actions
	return modeChanged || userESC;
}

void overallPiano::clearModeFlagChange(void)
{
	// Allow outside function to acknowledge modeChanged flag
	modeChanged = false;
}

bool overallPiano::readSong(std::ifstream &songToRead)
{
	// Read next line of song
	// Returns:
	//	true if no errors (has set songNotes array)
	//	false if error reading next line (generally end of file)
	int position;
	char peek;
	std::string nextLine;

	resetSongNotes();				// reset songNotes array

	if (!songToRead.eof())			// If it hasn't gotten to the end of the file
		peek = songToRead.peek();	//		Look at the next character
	else							// If it has reach EoF
	{
		this->setCurrentMode(3);
		return true;				//		return false
	}

	if (peek != '[') {								// If the next character is not the multi-note indicator
		std::getline(songToRead, nextLine, ' ');	//	Pull characters until the next space
		position = checkValidNote(nextLine);		//	Check that string to make sure it's a valid note
		if (position != -1) {						//	If the note is valid
			songNotes[position] = true;				//		Set the songNotes array appropriately
		}
	}
	else {											// Otherwise, the next character should be the multi-note indicator
		std::getline(songToRead, nextLine, ']');	//	Pull characters until the close of the multi-note indicator (i.e., ']')
		nextLine = nextLine.substr(1, nextLine.length()) + ' ';	// remove leading '[' and add trailing space to enforce trailing space after each note

		std::size_t location = nextLine.find(' ');	// Find the first note's ending space
		while (location <= nextLine.length()) {		// For each valid note to be played simultaneously
			position = checkValidNote(nextLine.substr(0, location));	// Check note
			if (position != -1) {										// Set the songNotes array
				songNotes[position] = true;
			}
			nextLine = nextLine.substr(location + 1, nextLine.length());	// Remove the note from the read line
			location = nextLine.find(' ');								// Look for subsequent notes
		}
		songToRead.get();							// Safely increment pointer for trailing ' ' after ']'
	}
	return true;									// Successful read
}

void overallPiano::getUserInput(void)
{
	// Get all user inputs

	checkKeyStates();	// Check keyboard
	checkModeChanged();	// Check mouse

						// break early?
	FsPollDevice();
	if (FSKEY_ESC == FsInkey())
		userESC = true;
}

void overallPiano::getModeButtonPosition(const int mode, int & centerX, int & centerY, int & radius)
{
	// Load mode button coordinates into parameters

	centerX = modeButtonX[mode];
	centerY = modeButtonY[mode];
	radius = modeButtonRadius;
}


void overallPiano::songNextNotes(bool whereToStore[15])
{
	// deep copy songNotes to whereToStore
	std::memcpy(whereToStore, songNotes, 15);
}

void overallPiano::userNotesToPlay(bool whereToStore[15])
{
	// deep copy userKeyFirstPressed to whereToStore
	std::memcpy(whereToStore, userKeyFirstPressed, 15);
}

void overallPiano::userNotesToDisplay(bool whereToStore[15])
{
	// deep copy userKeyDepressed to whereToStore
	std::memcpy(whereToStore, userKeyDepressed, 15);
}

void overallPiano::printUsersInput(void)
{
	int i;

	// Print current keyboard button pressed states
	for (i = 0; i < 15; i++) {
		std::cout << userKeyDepressed[i];
	}
	std::cout << std::endl;

	// ID buttons that are pressed the first time this iteration
	for (i = 0; i < 15; i++) {
		if (userKeyFirstPressed[i])
			std::cout << "*";
		else
			std::cout << " ";
	}
	std::cout << std::endl;
}

