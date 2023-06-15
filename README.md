# MF.OV.MVR

Example of a fixture defined in a MVR file (contains some, but not all properties):
```xml
<Fixture name="Sharpy" uuid="C63B1F8D-6DAD-438C-9228-E33C6EF2947E">
    <Matrix>{-1.000000,0.000000,0.000000}{0.000000,-1.000000,-0.000000}{0.000000,0.000000,1.000000}{-766.333333,4572.000000,7620.000000}</Matrix>
    <GDTFSpec>Clay Paky@Sharpy [Bulb=MSD Platinum 5R 189W].gdtf</GDTFSpec>
    <GDTFMode>Vect</GDTFMode>
    <Addresses>
        <Address break="0">21</Address>
    </Addresses>
    <FixtureID>102</FixtureID>
    <UnitNumber>0</UnitNumber>
    <FixtureTypeId>0</FixtureTypeId>
    <CustomId>0</CustomId>
    <Color>0.312712,0.329008,100.000000</Color>
    <CastShadow>false</CastShadow>
    <Mappings/>
</Fixture>
```

Some notes on the properties:
- Matrix is in milimeters (applies to the last part, the translation).
- Color is in [CIE 1931 color space](https://en.wikipedia.org/wiki/CIE_1931_color_space) and represent the color of a color gel or similar apparatus and not of the fixture itself.

# How to

```
> link_app.bat --app create
> app\omni.create.bat --ext-folder exts --enable mf.ov.mvr
```
1. Select a folder in the Content tab (this is required because we're hooked into the import system which expects that it will write the files in the project folder and reference them in the scene instead of simply creating new scene elements from the reading of a file)
2. Click the "+ import" button and select "External Assets (FBX, OBJ...)
3. Choose a mvr file and wait for it to import
4. This will create a scope named "MVR" under the default prim
5. Inside the MVR scope, you'll find other scope representing the layers of a MVR scene
6. Inside them you'll find each Fixture represented by an xform
7. xform are named using their name and their uuid to ensure unique naming
8. xform also have custom properties (see Raus USD Properties) using the following convention: `mf:mvr:property`
