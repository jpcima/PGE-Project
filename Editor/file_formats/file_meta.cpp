/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "file_formats.h"

MetaData FileFormats::ReadNonSMBX64MetaData(QString RawData, QString filePath)
{
    int str_count=0;        //Line Counter
    QString line;           //Current Line data


    MetaData FileData;
    FileData.script = new ScriptHolder();

    QString errorString;

    ///////////////////////////////////////Begin file///////////////////////////////////////
    PGEFile pgeX_Data(RawData);
    pgeX_Data.setRawData( RawData );

    if( !pgeX_Data.buildTreeFromRaw() )
    {
        errorString = pgeX_Data.lastError();
        goto badfile;
    }

    for(int section=0; section<pgeX_Data.dataTree.size(); section++) //look sections
    {
        if(pgeX_Data.dataTree[section].name=="JOKES")
        {
            #ifndef PGE_ENGINE
            if(!pgeX_Data.dataTree[section].data.isEmpty())
                if(!pgeX_Data.dataTree[section].data[0].values.isEmpty())
                    QMessageBox::information(nullptr, "Jokes",
                            pgeX_Data.dataTree[section].data[0].values[0].value,
                            QMessageBox::Ok);
            #endif
        }
        else
        if(pgeX_Data.dataTree[section].name=="META_BOOKMARKS")
        {
            if(pgeX_Data.dataTree[section].type!=PGEFile::PGEX_Struct)
            {
                errorString=QString("Wrong data syntax, section [%1]").arg(pgeX_Data.dataTree[section].name);
                goto badfile;
            }

            for(int sdata=0;sdata<pgeX_Data.dataTree[section].data.size();sdata++)
            {
                if(pgeX_Data.dataTree[section].data[sdata].type!=PGEFile::PGEX_Struct)
                {
                    errorString=QString("Wrong data syntax, section [%1], data line %2")
                            .arg(pgeX_Data.dataTree[section].name)
                            .arg(sdata);
                    goto badfile;
                }

                PGEFile::PGEX_Item x = pgeX_Data.dataTree[section].data[sdata];

                Bookmark meta_bookmark;
                meta_bookmark.bookmarkName = "";
                meta_bookmark.x = 0;
                meta_bookmark.y = 0;

                for(int sval=0;sval<x.values.size();sval++) //Look markers and values
                {
                    PGEFile::PGEX_Val v = x.values[sval];
                    errorString=QString("Wrong data syntax, section [%1], data line %2, marker %3")
                            .arg(pgeX_Data.dataTree[section].name)
                            .arg(sdata)
                            .arg(sval);

                      if(v.marker=="BM") //Bookmark name
                      {
                          if(PGEFile::IsQStr(v.value))
                              meta_bookmark.bookmarkName = PGEFile::X2STR(v.value);
                          else
                              goto badfile;
                      }
                      else
                      if(v.marker=="X") // Position X
                      {
                          if(PGEFile::IsIntS(v.value))
                              meta_bookmark.x = v.value.toInt();
                          else
                              goto badfile;
                      }
                      else
                      if(v.marker=="Y") //Position Y
                      {
                          if(PGEFile::IsIntS(v.value))
                              meta_bookmark.y = v.value.toInt();
                          else
                              goto badfile;
                      }
                }
                FileData.bookmarks.push_back(meta_bookmark);
            }
        }
    }
    errorString="";
    ///////////////////////////////////////EndFile///////////////////////////////////////

    return FileData;

    badfile:    //If file format is not correct
    BadFileMsg(filePath+"\nError message: "+errorString, str_count, line);
    FileData.bookmarks.clear();

    return FileData;
}


QString FileFormats::WriteNonSMBX64MetaData(MetaData metaData)
{
    QString TextData;
    long i;

    //Bookmarks
    if(!metaData.bookmarks.isEmpty())
    {
        TextData += "META_BOOKMARKS\n";
        for(i=0;i<metaData.bookmarks.size(); i++)
        {
            //Bookmark name
            TextData += PGEFile::value("BM", PGEFile::qStrS(metaData.bookmarks[i].bookmarkName));
            TextData += PGEFile::value("X", PGEFile::IntS(metaData.bookmarks[i].x));
            TextData += PGEFile::value("Y", PGEFile::IntS(metaData.bookmarks[i].y));
            TextData += "\n";
        }
        TextData += "META_BOOKMARKS_END\n";
    }
    return TextData;
}

