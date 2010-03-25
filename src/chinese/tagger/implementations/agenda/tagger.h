/***************************************************************
 *
 * The tagger's agenda n chart implementation 
 *
 * Yue Zhang, 2007
 *
 ***************************************************************/

#ifndef TAGGER_AGENDACHART_IMPL_H
#define TAGGER_AGENDACHART_IMPL_H

#include "tagger_base.h"

namespace chinese {

namespace tagger {

class CSubStateItem : public CStateItem {

protected:
   SCORE_TYPE sub_score; // score of the last word

public:
   void follow(const CSubStateItem &gold) {
      assert(size()<=gold.size());
      if (size() < gold.size()) {
         if (size() == 0) {
            append(0, CTag::NONE);
            return;
         }
         const unsigned &last_character = getWordEnd(size()-1);
         const unsigned &gold_last = gold.getWordEnd(size()-1);
         assert(last_character<=gold_last);
         if (last_character<gold_last) {
            replace(last_character+1, CTag::NONE);
            return;
         }
         else {
            append(last_character+1, CTag::NONE);
            setTag(size()-2, gold.getTag(size()-2).code());
            return;
         }
      }
      else { // size() == gold.size()
         setTag(size()-1, gold.getTag(size()-1).code());
         return;
      }
   }

};

}

/*===============================================================
 *
 * The implementation-specific part of tagger is defined here.
 *
 *==============================================================*/

class CTagger : public CTaggerBase {

protected:
   CAgendaBeam<tagger::CSubStateItem> m_Agenda;
   CWordCache m_WordCache;
   tagger::CSubStateItem m_goldState;
   unsigned m_nScoreIndex;
   bool m_bTrainingError;

public:
   CTagger(const string &sFeatureDBPath, bool bTrain, unsigned long nMaxSentSize, const string &sKnowledgePath, bool bSegmentationRules) : m_Agenda(tagger::AGENDA_SIZE) , CTaggerBase(sFeatureDBPath, bTrain, nMaxSentSize, sKnowledgePath, bSegmentationRules) , m_WordCache(nMaxSentSize) {
      if (bTrain) m_nScoreIndex = CScore<tagger::SCORE_TYPE>::eNonAverage; else m_nScoreIndex = CScore<tagger::SCORE_TYPE>::eAverage;
      ASSERT(sizeof(unsigned long long)>=CTag::SIZE, "The tagger requires the size of unsigned-long greater than" << CTag::SIZE); // tag dict
   }
   virtual ~CTagger() {}
   
protected:
   void loadKnowledge(const string &sKnowledgePath) {
      cout << "Loading knowledge ... ";
      m_weights->newKnowledge();
      ifstream ifs(sKnowledgePath.c_str());
      if (!ifs) THROW("Knowledge file " << sKnowledgePath << " is not accessible.");
      ifs >> (*m_weights->m_Knowledge); 
      ifs.close();
      cout << "done." << endl;
   }
   inline bool canAssignTag(const CWord &word, const unsigned long &tag) {
      return ( m_weights->m_mapWordFrequency.find( word, 0 ) < 
                  m_weights->m_nMaxWordFrequency/5000+5 && 
               PENN_TAG_CLOSED[ tag ] == false  ) ||
             m_weights->m_mapTagDictionary.lookup( word, tag );
   }
   inline bool canStartWord(const CStringVector &sentence, const unsigned long &tag, const unsigned long &index) {
      if (PENN_TAG_CLOSED[ tag ]) {
         static int tmp_i;
         for (tmp_i=0; tmp_i<m_weights->m_maxLengthByTag[tag]; ++tmp_i) {
            if ( m_weights->m_mapCanStart.lookup( m_WordCache.find( index, index+i, sentence ), tag ) == 0 ) 
               return false;
         }
      }
      return true;
   }

public:
   enum SCORE_UPDATE {eSubtract=0, eAdd};

   virtual bool train(const CStringVector *sentence, const CTwoStringVector *correct);
   virtual void tag(const CStringVector *sentence, CTwoStringVector *retval, double *out_scores=NULL, unsigned long nBest=1, const CBitArray *prunes=NULL);

   void finishTraining(unsigned long nTotalNumberOfTrainingExamples) { 
      m_weights->computeAverageFeatureWeights(nTotalNumberOfTrainingExamples);
      m_weights->saveScores(); 
   }

public:
   tagger::SCORE_TYPE getGlobalScore(const CTwoStringVector* tagged) {
      THROW("This method is not supported in this implementation.");
   }
   void updateScores(const CTwoStringVector* tagged, const CTwoStringVector* correct, unsigned long round) {
      THROW("This method is not supported in this implementation.");
   }
   tagger::SCORE_TYPE getOrUpdateLocalScore(const CStringVector *tagged, const tagger::CSubStateItem *item, unsigned long index, tagger::SCORE_TYPE amount=0, unsigned long round=0) {
      THROW("This method is not supported in this implementation.");
   }

   inline void updateScoreForState(const CStringVector *sentence, const tagger::CSubStateItem *item, const tagger::SCORE_TYPE &amount) {
      static unsigned tmp_i, tmp_j;
      for (tmp_i=-1; tmp_i<item->size(); ++tmp_i) {
         getOrUpdateSeparateScore(sentence, item, tmp_i, amount, m_nTrainingRound);
         for (tmp_j=item->getWordStart(tmp_i)+1; tmp_j<item->getWordEnd(tmp_i)+1; ++tmp_j)
            getOrUpdateAppendScore(sentence, item, tmp_i, tmp_j, amount, m_nTrainingRound);
      }
   }
   tagger::SCORE_TYPE getOrUpdateSeparateScore(const CStringVector *tagged, const tagger::CSubStateItem *item, unsigned long index, tagger::SCORE_TYPE amount=0, unsigned long round=0);
   tagger::SCORE_TYPE getOrUpdateAppendScore(const CStringVector *tagged, const tagger::CSubStateItem *item, unsigned long index, unsigned long char_index, tagger::SCORE_TYPE amount=0, unsigned long round=0);

};

} // namespace chinese





#endif