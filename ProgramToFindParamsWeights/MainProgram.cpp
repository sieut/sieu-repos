#include "weightClasses.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <ios>
#include <iomanip>
using namespace std;

void CreateTeamVector(vector<TeamWithNeighbor>& teamData);
void CreateMatchVector(vector<MatchWithWeight>& matchData);
void CreateRosterVector(vector<RosterInfo>& rosterData);
void CalCulateExpectedScore(const Team& tA, const Team& tB, double& expectedA, double& expectedB);
double UpdateRating(const MatchWithWeight& match, vector<TeamWithNeighbor>& teamData, double eta, double lambda);
double UpdateRatingRMSE(const MatchWithWeight& match, vector<TeamWithNeighbor>& teamData, double eta, double lambda);
double LearningRate(int currentIter, int maxIter);
double NeighborLoss(vector<TeamWithNeighbor>& teamData, double lambda);
void MatchAddNeighbor (vector<TeamWithNeighbor>& teamData, vector<MatchWithWeight>& matchData);
//implementation in another file

int main()
{
	vector<TeamWithNeighbor> teamData;
	CreateTeamVector(teamData);
	vector<MatchWithWeight> matchData;
	CreateMatchVector(matchData);
    MatchAddNeighbor(teamData, matchData);
    //vector<RosterInfo> rosterData;
    //CreateRosterVector(rosterData);
  
    cout << "Tmax: ";
    cin >> MatchWithWeight::tmax;

    string outputFileName;
    cout << "Name of output file (no .txt): ";
    cin >> outputFileName;
    outputFileName += ".txt";
    ofstream outFile(outputFileName.c_str());

    double eta, lambda; 
    int maxIter;
    double lambdaStart, lambdaStop;
    int etaStep, lambdaStep;


    cout << "maxIter-" << endl;
    cin >> maxIter;
    cout << "lambda-" << endl;
    cout << "start: ";
    cin >> lambdaStart;
    cout << "stop: ";
    cin >> lambdaStop;
    cout << "step: ";
    cin >> lambdaStep;

    // 2nd process
    double keepRmse = 1000.0;
    double rmse;
    double keepFinalEta, keepFinalLambda;


    double lambdaEachStep = (lambdaStop - lambdaStart) / lambdaStep;
    if (lambdaStep == 1)
        lambdaEachStep = 50.0;

    double loseProb = 0.0;
    double lowestLoseProb = 100000.0;
    int keepIter = 0;
    double keepLambda = 0;
    double bestEta = 0.0;
    double loseProbEta = 10000.0;

    for (lambda = lambdaStart; lambda <= lambdaStop; lambda += lambdaEachStep)
    {
        loseProbEta = 100000.0;
        bestEta = 0.0;
        for (int iter = 1; iter <= maxIter; iter++)
        {
            loseProb = 0.0;
            //int rosterInfoIdx = 0;      // NEW: Index over roster changes vector
            eta = LearningRate(iter, maxIter);

            for (int i = 0; i < 8000; i++)
            {
                /*while (rosterData.at(rosterInfoIdx).Date() == matchData.at(i).Date()) {     // NEW: Adjust teams' numPlay due to roster change
                    teamData.at(rosterData.at(rosterInfoIdx).Index()).AdjustNumPlay();
                    rosterInfoIdx += 1;            
                    }*/
                loseProb += UpdateRating(matchData.at(i), teamData, eta, lambda);
                //cout << loseProb << endl;
            }

            //cout << loseProb << endl;
            loseProb += NeighborLoss(teamData, lambda);
            //cout << loseProb << endl;

            if (loseProb < lowestLoseProb)
            {
                lowestLoseProb = loseProb;
                keepIter = iter;
                keepLambda = lambda;
            }

            if (loseProb < loseProbEta)
            {
                bestEta = eta;
                loseProbEta = loseProb;
            }

            //outFile << fixed << setprecision(7);
            //outFile << LearningRate(keepIter, maxIter) << " " << lambda << " " << loseProb << endl;

            //Reset Team'stats
            for (int i = 0; i < teamData.size(); i++)
            {
                teamData.at(i).Reset();
            }
        }

        outFile << lambda << " " << bestEta << " " << loseProbEta << endl;
        for (int i = 8000; i < matchData.size(); i++)
        {
            rmse += UpdateRating(matchData.at(i), teamData, bestEta, lambda);
        }
        rmse = sqrt(rmse / teamData.size());

        if (rmse < keepRmse)
        {
            keepRmse = rmse;
            keepFinalLambda = lambda;
            keepFinalEta = bestEta;
        }

    }

    cout << "Lowest RMSE: " << keepRmse << endl;
    cout << "at eta: " << keepFinalEta << " at lambda: " << keepFinalLambda << endl;


/*    int dummy;

    for (int i = 0; i < 3; i++)
    {
        UpdateRating(matchData.at(i), teamData, 0.735, 0.77);
        outFile << "Match#" << i + 1 << endl;
        outFile << teamData.at(matchData.at(i).WinTeam()) << endl;
        outFile << teamData.at(matchData.at(i).LoseTeam()) << endl;
//        if (matchData.at(i).LoseTeam() == 3 || matchData.at(i).WinTeam() == 3)
//        {
//            outFile << "Match#" << i + 1 << endl;
//            outFile << teamData.at(matchData.at(i).WinTeam()) << endl;
//            outFile << teamData.at(matchData.at(i).LoseTeam()) << endl;
//        }
    }*/
/*
    for (int i = 0; i < teamData.size(); i++)
    {
        outFile << teamData.at(i) << endl;
    }
    */

	return 0;
}

void MatchAddNeighbor (vector<TeamWithNeighbor>& teamData, vector<MatchWithWeight>& matchData)
{
    for (unsigned int i = 0; i < matchData.size(); i++) {
        teamData.at(matchData.at(i).WinTeam()).AddNeighbor(&teamData.at(matchData.at(i).LoseTeam()));
        teamData.at(matchData.at(i).LoseTeam()).AddNeighbor(&teamData.at(matchData.at(i).WinTeam()));
    }
    
}

double LearningRate(int currentIter, int maxIter) {
    return pow((1 + 0.1*maxIter) / (currentIter + 0.1*maxIter), 0.602);
}

void CalCulateExpectedScore(const TeamWithNeighbor& tA, const TeamWithNeighbor& tB, 
    double& expectedA, double& expectedB)
{
	expectedA = 1 / ( 1 + exp(tB.Rating() - tA.Rating()) );
	expectedB = 1 - expectedA;
}

//Return added lose probability
double UpdateRating(const MatchWithWeight& match, vector<TeamWithNeighbor>& teamData, 
    double eta, double lambda)
{
    //teamA = teamData.at(match.WinTeam), teamB = teamData.at(match.LostTeam)
    double expectedA, expectedB;
    TeamWithNeighbor *tA = &teamData.at(match.WinTeam());
    TeamWithNeighbor *tB = &teamData.at(match.LoseTeam());
    CalCulateExpectedScore(*tA, *tB, expectedA, expectedB);
/*    cout << "expectedA: " << expectedA << endl;
    cout << "expectedB: " << expectedB << endl;*/
    //tA->AddNeighbor(tB);
    //tB->AddNeighbor(tA);

    if (match.isTie())
    {
        //k1 weight is built in AddRating
        tA->AddRating(-eta * (match.Weight() * (expectedA - 0.5) * expectedA * (1 - expectedA) 
                + (lambda / tA->NumNeighbor()) * 
                (tA->Rating() - tA->AverageNeighbor())));
        tB->AddRating(-eta * (match.Weight() * (expectedB - 0.5) * expectedB * (1 - expectedB) 
                + (lambda / tB->NumNeighbor()) * 
                (tB->Rating() - tB->AverageNeighbor())));
        return match.Weight() * pow((expectedA - 0.5), 2);
    }
    else
    {
        tA->AddRating(-eta * (match.Weight() * (expectedA - 1) * expectedA * (1 - expectedA) 
                + (lambda / tA->NumNeighbor()) * 
                (tA->Rating() - tA->AverageNeighbor())));
        tB->AddRating(-eta * (match.Weight() * (expectedB - 0) * expectedB * (1 - expectedB) 
                + (lambda / tB->NumNeighbor()) * 
                (tB->Rating() - tB->AverageNeighbor())));
        return match.Weight() * pow((expectedA - 1.0), 2);
    }
}

double UpdateRatingRMSE(const MatchWithWeight& match, vector<TeamWithNeighbor>& teamData, double eta, double lambda)
{
    //teamA = teamData.at(match.WinTeam), teamB = teamData.at(match.LostTeam)
    double expectedA, expectedB;
    TeamWithNeighbor *tA = &teamData.at(match.WinTeam());
    TeamWithNeighbor *tB = &teamData.at(match.LoseTeam());
    CalCulateExpectedScore(*tA, *tB, expectedA, expectedB);
/*    cout << "expectedA: " << expectedA << endl;
    cout << "expectedB: " << expectedB << endl;*/
    //tA->AddNeighbor(tB);
    //tB->AddNeighbor(tA);

    if (match.isTie())
    {
        //k1 weight is built in AddRating
        tA->AddRating(-eta * (match.Weight() * (expectedA - 0.5) * expectedA * (1 - expectedA) 
                + (lambda / tA->NumNeighbor()) * 
                (tA->Rating() - tA->AverageNeighbor())));
        tB->AddRating(-eta * (match.Weight() * (expectedB - 0.5) * expectedB * (1 - expectedB) 
                + (lambda / tB->NumNeighbor()) * 
                (tB->Rating() - tB->AverageNeighbor())));
        return pow((expectedA - 0.5), 2);
    }
    else
    {
        tA->AddRating(-eta * (match.Weight() * (expectedA - 1) * expectedA * (1 - expectedA) 
                + (lambda / tA->NumNeighbor()) * 
                (tA->Rating() - tA->AverageNeighbor())));
        tB->AddRating(-eta * (match.Weight() * (expectedB - 0) * expectedB * (1 - expectedB) 
                + (lambda / tB->NumNeighbor()) * 
                (tB->Rating() - tB->AverageNeighbor())));
        return pow((expectedA - 1.0), 2);
    }

}

double NeighborLoss(vector<TeamWithNeighbor>& teamData, double lambda) {
    double result = 0.0;
    for (unsigned int i = 0; i < teamData.size(); i++) {
        //cout << teamData.at(i).Rating() << " " << teamData.at(i).AverageNeighbor() << endl;

        result += pow((teamData.at(i).Rating() - teamData.at(i).AverageNeighbor()), 2);
    }
    result *= lambda;
    return result;
}

void CreateTeamVector(vector<TeamWithNeighbor>& teamData)
{
    string inFileName = "teamList_updated.txt";
    //cout << "Print \"teamList.txt\": ";
    //cin >> inFileName;

    ifstream infile(inFileName.c_str());
    if (!infile.is_open())
    {
        cerr << "Cannot open team file." << endl;
        return;
    }
    string line;
    getline(infile, line);
    while (infile.good())
    {
        teamData.push_back(TeamWithNeighbor(line));
        getline(infile, line);
    }

    if (!infile.eof())
    {
        cerr << "Cannot read til the end (team)." << endl;
        return;
    }
}

void CreateMatchVector(vector<MatchWithWeight>& matchData)
{
    string inFileName = "matchIndex_updated.txt";
    //cout << "Print \"matchIndex.txt\": ";
    //cin >> inFileName;

    ifstream infile(inFileName.c_str());
    if (!infile.is_open())
    {
        cerr << "Cannot open match file." << endl;
        return;
    }
    string line;
    getline(infile, line);
    while (infile.good())
    {
        matchData.push_back(MatchWithWeight(line));
        getline(infile, line);
    }

    if (!infile.eof())
    {
        cerr << "Cannot read til the end (match)." << endl;
        return;
    }
}

void CreateRosterVector(vector<RosterInfo>& rosterData) {
    string inFileName = "rosterChange.txt";
    //cout << "Print \"rosterChange.txt\": ";
    //cin >> inFileName;

    ifstream infile(inFileName.c_str());
    if (!infile.is_open()) {
        cerr << "Cannot open roster file." << endl;
        return;
    }

    string line;
    getline(infile, line);
    while (infile.good()) {
        rosterData.push_back(RosterInfo(line));
        getline(infile, line);
    }

    if (!infile.eof()) {
        cerr << "Cannot read til the end." << endl;
        return;
    }
}

