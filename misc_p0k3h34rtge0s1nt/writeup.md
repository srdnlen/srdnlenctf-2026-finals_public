# P0k3h34rtge0s1nT

The challenge provides a Pokemon HeartGold modified ROM, modified to turn it into a geographic investigation. In the solve the player has to identify landmarks from partial descriptions, check their relationships on a map, and use public sources to work out where to look next.

The modified game contains short dialogues with uppercase words. Some belong to the solution; others are false friends placed both along the route and elsewhere. Finding an uppercase word tells us what to extract **if the dialogue is relevant**.

The useful tools are an emulator with the supplied ROM and save, Google, and a Pokemon wiki such as Bulbapedia. ROM analysis can help enumerate dialogue, but the intended solve is to connect places and stories. 

## Establishing the Starting Point

The entry clue is:

> The challenge begins where the sea has already tested you, and where the sky first became a shortcut.

The two halves describe a journey and a newly available way to travel. In HeartGold, a good first search is:

```text
Pokemon HeartGold where do you get Fly
```

The results point to Cianwood City: Chuck's wife gives the player HM02, Fly, after Chuck is defeated. Cianwood's sea access also matches the first half of the clue.  [Cianwood City](https://bulbapedia.bulbagarden.net/wiki/Cianwood_City), [Chuck's wife](https://bulbapedia.bulbagarden.net/wiki/Chuck%27s_wife).


## 1. Cianwood:

The boy's dialogue reads:

```text
Let FLY cross old roads.
Fire made a tower legend.
```

Record **FLY**. The remaining lines describe the next landmark: a tower whose history involves fire.

Useful Google queries:

```text
Pokemon HeartGold tower fire legend
site:bulbapedia.bulbagarden.net Burned Tower
```

The Burned Tower is in Ecruteak City. Its history connects a destructive fire, three Pokemon that died, and Ho-Oh restoring them to life ([Burned Tower](https://bulbapedia.bulbagarden.net/wiki/Burned_Tower)).

Use the wiki's **HeartGold/SoulSilver layout**, then compare it with the in-game map. Matching the correct generation matters when using building positions and paths as landmarks. The next search area is Ecruteak and its tower surroundings.

Cianwood also contains "false" candidates such as `WAVES`, `BADGE`, and `MEDICINE`.

## 2. Ecruteak

The old man's dialogue reads:

```text
A RAINBOW Pokemon fell.
West, a sea tower healed.
```

Record **RAINBOW**. To check the connection with the previous clue, search something like:

```text
Pokemon rainbow Pokemon burned tower Ho-Oh
```

Ho-Oh is classified as the Rainbow Pokemon and is associated with the tower's resurrection legend. The dialogue's wording is compressed: the relevant lore is Ho-Oh descending and reviving the Pokemon ([Ho-Oh](https://bulbapedia.bulbagarden.net/wiki/Ho_Oh)).

The next destination has three constraints: it lies to the west, involves a tower by the sea, and has a connection to healing. Search those concepts together:

```text
HeartGold sick Pokemon
Olivine lighthouse medicine
```

Olivine's lighthouse fits. Amphy, the Ampharos that provides its light, becomes ill; Jasmine asks the player to obtain medicine from Cianwood. The thing being healed is the Pokemon powering the lighthouse ([Olivine Lighthouse](https://bulbapedia.bulbagarden.net/wiki/Glitter_Lighthouse)).

From a geographical standpoint, Route 38 leads west from Ecruteak toward Route 39, which continues to Olivine ([Route 39](https://bulbapedia.bulbagarden.net/wiki/Johto_Route_39)).


## 3. Olivine

In Olivine, the relevant outdoor sailor says:

```text
The Lighthouse is a BEACON.
Ships trust its light. Recall Red fury...
```

Record **BEACON**. The first sentence confirms the lighthouse association. The next lead is "Red fury".

Useful searches:

```text
Pokemon HeartGold red angry Pokemon
HeartGold red Gyarados Lake of Rage
```

The red Gyarados at the Lake of Rage matches the color and the anger association. The game's story also connects it to Team Rocket's forced-evolution radio broadcast ([Red Gyarados](https://bulbapedia.bulbagarden.net/wiki/The_Red_Gyarados)).

That suggests a second search:

```text
HeartGold Lake of Rage radio source hideout
```

The source is Team Rocket HQ beneath the souvenir shop in **Mahogany Town**. The lake identifies the event; investigating its cause identifies the town containing the next clue ([Team Rocket HQ](https://bulbapedia.bulbagarden.net/wiki/Team_Rocket%27s_Hideout)).


## 4. Mahogany

The outdoor old man's post event dialogue reads:

```text
Below town: HAIL Giovanni.
Where do they shed skin, I wonder.
```

Record **HAIL**. The recognizable phrase is an especially useful search anchor:

```text
"Hail Giovanni" HeartGold
```

Petrel's HeartGold/SoulSilver dialogue identifies this as the password associated with the hideout's transmitter room. This corroborates the previous step's destination. Extract the uppercase word `HAIL` ([Petrel's quotes](https://bulbapedia.bulbagarden.net/wiki/Petrel/Quotes)).

The second sentence changes the research task from identifying a place to identifying a Pokemon:

```text
Pokemon HeartGold "shed skin"
Dratini HeartGold location
```

Dratini's HeartGold Pokedex entry describes repeated shedding as it grows. Its HeartGold/SoulSilver locations include Dragon's Den. Dratini in this context is of the possible candidates (by exclusion, it's one of the most likely compared to other that do not have a specific/iconic location, like Metapod) [Dratini](https://bulbapedia.bulbagarden.net/wiki/Dratini_%28Pokemon%29).


The intended pivot is therefore:

```text
shedding skin -> Dratini -> Dragon's Den
```


## 5. Dragon's Den

Now refine the search from a city to an exact landmark:

```text
HeartGold Dragon's Den entrance behind gym
Blackthorn City map HeartGold Dragon's Den
```

Blackthorn's map places the entrance behind the Gym, across a pond. Clair's dialogue also directs the player to the den and its shrine as a test of worth. These details connect the map position with the previous riddle ([Blackthorn City](https://bulbapedia.bulbagarden.net/wiki/Fusube_City), [Clair's quotes](https://bulbapedia.bulbagarden.net/wiki/Clair/Quotes)).

Read the sign near the entrance:


```text
Dragon Shrine
A shrine revering the Dragon Pokémon
said to have molted in Dragon’s DEN.
```
This new text reinforces both the molting connection and the same final keyword; so we record **DEN**. 

## Assembling the Flag

The route gives the order:

| Step | Location | Extracted word |
| --- | --- | --- |
| 1 | Cianwood City | `FLY` |
| 2 | Ecruteak City | `RAINBOW` |
| 3 | Olivine City | `BEACON` |
| 4 | Mahogany Town | `HAIL` |
| 5 | Blackthorn | `DEN` |

Using the flag convention established for this challenge:

```text
srdnlen{FLY_RAINBOW_BEACON_HAIL_DEN}
```
