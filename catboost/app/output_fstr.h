#pragma once

#include <catboost/libs/algo/calc_fstr.h>
#include <catboost/libs/algo/tree_print.h>
#include <catboost/libs/fstr/doc_fstr.h>

#include <util/stream/file.h>

inline void OutputFstr(const TFeaturesLayout& layout,
                       const yvector<std::pair<double, TFeature>>& effect,
                       const TString& path) {
    TAdaptiveFileOutput out(path);
    for (const auto& effectWithSplit : effect) {
        out << effectWithSplit.first << "\t" << BuildDescription(layout, effectWithSplit.second) << Endl;
    }
}

inline void OutputRegularFstr(const TFeaturesLayout& layout,
                              const yvector<TFeatureEffect>& regularEffect,
                              const TString& path) {
    TAdaptiveFileOutput out(path);
    for (const auto& initialFeatureScore : regularEffect) {
        out << initialFeatureScore.Score << "\t" <<
               BuildFeatureDescription(layout,
                                       initialFeatureScore.Feature.Index,
                                       initialFeatureScore.Feature.Type) << Endl;
    }
}

inline void OutputInteraction(const TFeaturesLayout& layout,
                       const yvector<TInternalFeatureInteraction>& interactionValues,
                       const TString& path) {
    TAdaptiveFileOutput out(path);
    for (const auto& interaction : interactionValues) {
        out << interaction.Score << "\t" << BuildDescription(layout, interaction.FirstFeature) <<
            "\t" << BuildDescription(layout, interaction.SecondFeature) << Endl;
    }
}

inline void OutputRegularInteraction(const TFeaturesLayout& layout,
                              const yvector<TFeatureInteraction>& interactionValues,
                              const TString& path) {
    TAdaptiveFileOutput out(path);
    for (const auto& interaction : interactionValues) {
        out << interaction.Score << "\t" <<
            BuildFeatureDescription(layout,
                                    interaction.FirstFeature.Index,
                                    interaction.FirstFeature.Type) << "\t" <<
            BuildFeatureDescription(layout,
                                   interaction.SecondFeature.Index,
                                   interaction.SecondFeature.Type) << Endl;
    }
}

inline void OutputFeatureImportanceMatrix(const yvector<yvector<double>>& featureImportance,
                                          const TString& path) {
    Y_ASSERT(!featureImportance.empty());
    TAdaptiveFileOutput out(path);
    const int docCount = featureImportance[0].ysize();
    const int featureCount = featureImportance.ysize();
    for (int docId = 0; docId < docCount; ++docId) {
        for (int featureId = 0; featureId < featureCount; ++featureId) {
            out << featureImportance[featureId][docId   ] << (featureId + 1 == featureCount ? '\n' : '\t');
        }
    }
}

inline void CalcAndOutputFstr(const TFullModel& model,
                              const TPool& pool,
                              const TString* regularFstrPath,
                              const TString* internalFstrPath,
                              int threadCount = 1) {
    CB_ENSURE(!pool.Docs.empty(), "Pool should not be empty");
    int featureCount = pool.Docs[0].Factors.ysize();
    int catFeaturesCount = pool.CatFeatures.ysize();
    int floatFeaturesCount = featureCount - catFeaturesCount;
    TFeaturesLayout layout(featureCount, pool.CatFeatures, pool.FeatureId);

    yvector<std::pair<double, TFeature>> internalEffect = CalcFeatureEffect(model, pool, threadCount);
    if (internalFstrPath != nullptr && !internalFstrPath->empty()) {
        OutputFstr(layout, internalEffect, *internalFstrPath);
    }

    if (regularFstrPath != nullptr && !regularFstrPath->empty()) {
        yvector<TFeatureEffect> regularEffect = CalcRegularFeatureEffect(internalEffect, catFeaturesCount, floatFeaturesCount);
        OutputRegularFstr(layout, regularEffect, *regularFstrPath);
    }
}

inline void CalcAndOutputInteraction(const TFullModel& model,
                              const TPool& pool,
                              const TString* regularFstrPath,
                              const TString* internalFstrPath) {
    CB_ENSURE(!pool.Docs.empty(), "Pool should not be empty");
    int featureCount = pool.Docs[0].Factors.ysize();
    TFeaturesLayout layout(featureCount, pool.CatFeatures, pool.FeatureId);

    yvector<TInternalFeatureInteraction> internalInteraction = CalcInternalFeatureInteraction(model);
    if (internalFstrPath != nullptr) {
        OutputInteraction(layout, internalInteraction, *internalFstrPath);
    }

    if (regularFstrPath != nullptr) {
        yvector<TFeatureInteraction> interaction = CalcFeatureInteraction(internalInteraction, layout);
        OutputRegularInteraction(layout, interaction, *regularFstrPath);
    }
}

inline void CalcAndOutputDocFstr(const TFullModel& model,
                              const TPool& pool,
                              const TString& docFstrPath,
                              int threadCount) {
    CB_ENSURE(!pool.Docs.empty(), "Pool should not be empty");
    int featureCount = pool.Docs[0].Factors.ysize();
    TFeaturesLayout layout(featureCount, pool.CatFeatures, pool.FeatureId);

    yvector<yvector<double>> effect = CalcFeatureImportancesForDocuments(model, pool, threadCount);
    OutputFeatureImportanceMatrix(effect, docFstrPath);
}
