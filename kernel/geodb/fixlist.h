#pragma once

#include <kernel/search_types/search_types.h>

namespace NGeoDB {
    namespace NDetail {

        struct TFixBest {
            TCateg From;
            TCateg Best;
        };

        struct TFixBetter {
            TCateg From;
            TCateg Better;
            TCateg Worse;
        };

        constexpr TFixBest FIX_BEST[] = {
            {204, 10430}, // from Spain Valencia!
        };
        constexpr TFixBetter FIX_BETTER[] = {
            {225, 10129, 113239}, // from Russia Philadelphia (USA, PA) better then Philadelphia (USA, MS)
            {225, 10129, 104520}, // from Russia Philadelphia (USA, PA) better then England
            {225, 10129, 103751}, // from Russia Philadelphia (USA, PA) better then Brandenburg
            {225, 10129, 104218}, // from Russia Philadelphia (USA, PA) better then Calabria
            {225, 10129,  10013}, // from Russia Philadelphia (USA, PA) better then Jamaica
            {225, 10129,  21131}, // from Russia Philadelphia (USA, PA) better then Costa Rica
            {225, 10129,  29321}, // from Russia Philadelphia (USA, PA) better then Arkansas State
            {225, 10129,  29330}, // from Russia Philadelphia (USA, PA) better then Illinois Starte
            {225, 10129,    202}, // from Russia Philadelphia (USA, PA) better then New York State
            {225, 10129,  29331}, // from Russia Philadelphia (USA, PA) better then Indiana State
            {225, 10129,  29348}, // from Russia Philadelphia (USA, PA) better then New Mexico State
            {225, 10129,  29350}, // from Russia Philadelphia (USA, PA) better then North Carolina State
            {225, 10129,  29359}, // from Russia Philadelphia (USA, PA) better then Tennessee State
            {225, 10129,  21191}, // from Russia Philadelphia (USA, PA) better then Colombia
            {225, 10129, 123613}, // from Russia Philadelphia (USA, PA) better then Filadelfia from Boqueron
            {225, 10129, 145166}, // from Russia Philadelphia (USA, PA) better then Vibo Valentia
            {225, 10129, 145384}, // from Russia Philadelphia (USA, PA) better then Sunderland
            {225, 10994,  27712}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 118416}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 118417}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 126397}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 131448}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 131974}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 133799}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 134518}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 136931}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 131974}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 137753}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 139834}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 142945}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 148254}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 157948}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 158556}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 158947}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 159222}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 162398}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 162998}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 163618}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10994, 163846}, //  from Russia Krasnaya Polyana (Sochi) is the best
            {225, 10987, 10257}, // from Russia Russian Armavir is better than Armenian
            {225, 202, 29349}, // from Russia New York city (USA) better then New York (state)
            {10000, 24, 47}, // Velikiy Novgorod is better than Nizhny Novgorod from everywhere
            {40, 47, 24}, // except Volga Federal District
        };
    } // namespace NDetail
} // namespace NGeoDB
